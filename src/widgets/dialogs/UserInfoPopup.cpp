#include "UserInfoPopup.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/IvrApi.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clipboard.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "util/PostToThread.hpp"
#include "util/StreamerMode.hpp"
#include "widgets/Label.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/Window.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/Line.hpp"
#include "widgets/splits/Split.hpp"

#include <QCheckBox>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>

const QString TEXT_VIEWS("Views: %1");
const QString TEXT_FOLLOWERS("Followers: %1");
const QString TEXT_CREATED("Created: %1");
const QString TEXT_TITLE("%1's Usercard - #%2");
#define TEXT_USER_ID "ID: "
#define TEXT_UNAVAILABLE "(not available)"

namespace chatterino {
namespace {
    Label *addCopyableLabel(LayoutCreator<QHBoxLayout> box, const char *tooltip,
                            Button **copyButton = nullptr)
    {
        auto label = box.emplace<Label>();
        auto button = box.emplace<Button>();
        if (copyButton != nullptr)
        {
            button.assign(copyButton);
        }
        button->setPixmap(getApp()->themes->buttons.copy);
        button->setScaleIndependantSize(18, 18);
        button->setDim(Button::Dim::Lots);
        button->setToolTip(tooltip);
        QObject::connect(
            button.getElement(), &Button::leftClicked,
            [label = label.getElement()] {
                auto copyText = label->property("copy-text").toString();

                crossPlatformCopy(copyText.isEmpty() ? label->getText()
                                                     : copyText);
            });

        return label.getElement();
    };

    bool checkMessageUserName(const QString &userName, MessagePtr message)
    {
        if (message->flags.has(MessageFlag::Whisper))
            return false;

        bool isSubscription = message->flags.has(MessageFlag::Subscription) &&
                              message->loginName.isEmpty() &&
                              message->messageText.split(" ").at(0).compare(
                                  userName, Qt::CaseInsensitive) == 0;

        bool isModAction =
            message->timeoutUser.compare(userName, Qt::CaseInsensitive) == 0;
        bool isSelectedUser =
            message->loginName.compare(userName, Qt::CaseInsensitive) == 0;

        return (isSubscription || isModAction || isSelectedUser);
    }

    ChannelPtr filterMessages(const QString &userName, ChannelPtr channel)
    {
        LimitedQueueSnapshot<MessagePtr> snapshot =
            channel->getMessageSnapshot();

        ChannelPtr channelPtr(
            new Channel(channel->getName(), Channel::Type::None));

        for (size_t i = 0; i < snapshot.size(); i++)
        {
            MessagePtr message = snapshot[i];
            if (checkMessageUserName(userName, message))
            {
                channelPtr->addMessage(message);
            }
        }

        return channelPtr;
    };

    const auto borderColor = QColor(255, 255, 255, 80);

    int calculateTimeoutDuration(TimeoutButton timeout)
    {
        static const QMap<QString, int> durations{
            {"s", 1}, {"m", 60}, {"h", 3600}, {"d", 86400}, {"w", 604800},
        };
        return timeout.second * durations[timeout.first];
    }

}  // namespace

#ifdef Q_OS_LINUX
FlagsEnum<BaseWindow::Flags> userInfoPopupFlags{BaseWindow::Dialog,
                                                BaseWindow::EnableCustomFrame};
FlagsEnum<BaseWindow::Flags> userInfoPopupFlagsCloseAutomatically{
    BaseWindow::EnableCustomFrame};
#else
FlagsEnum<BaseWindow::Flags> userInfoPopupFlags{BaseWindow::EnableCustomFrame};
FlagsEnum<BaseWindow::Flags> userInfoPopupFlagsCloseAutomatically{
    BaseWindow::EnableCustomFrame, BaseWindow::Frameless,
    BaseWindow::FramelessDraggable};
#endif

UserInfoPopup::UserInfoPopup(bool closeAutomatically, QWidget *parent)
    : BaseWindow(closeAutomatically ? userInfoPopupFlagsCloseAutomatically
                                    : userInfoPopupFlags,
                 parent)
    , hack_(new bool)
    , dragTimer_(this)
{
    this->setWindowTitle("Usercard");
    this->setStayInScreenRect(true);

    if (closeAutomatically)
        this->setActionOnFocusLoss(BaseWindow::Delete);
    else
        this->setAttribute(Qt::WA_DeleteOnClose);

    HotkeyController::HotkeyMap actions{
        {"delete",
         [this](std::vector<QString>) -> QString {
             this->deleteLater();
             return "";
         }},
        {"scrollPage",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() == 0)
             {
                 qCWarning(chatterinoHotkeys)
                     << "scrollPage hotkey called without arguments!";
                 return "scrollPage hotkey called without arguments!";
             }
             auto direction = arguments.at(0);

             auto &scrollbar = this->ui_.latestMessages->getScrollBar();
             if (direction == "up")
             {
                 scrollbar.offset(-scrollbar.getLargeChange());
             }
             else if (direction == "down")
             {
                 scrollbar.offset(scrollbar.getLargeChange());
             }
             else
             {
                 qCWarning(chatterinoHotkeys) << "Unknown scroll direction";
             }
             return "";
         }},
        {"execModeratorAction",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.empty())
             {
                 return "execModeratorAction action needs an argument, which "
                        "moderation action to execute, see description in the "
                        "editor";
             }
             auto target = arguments.at(0);
             QString msg;

             // these can't have /timeout/ buttons because they are not timeouts
             if (target == "ban")
             {
                 msg = QString("/ban %1").arg(this->userName_);
             }
             else if (target == "unban")
             {
                 msg = QString("/unban %1").arg(this->userName_);
             }
             else
             {
                 // find and execute timeout button #TARGET

                 bool ok;
                 int buttonNum = target.toInt(&ok);
                 if (!ok)
                 {
                     return QString("Invalid argument for execModeratorAction: "
                                    "%1. Use "
                                    "\"ban\", \"unban\" or the number of the "
                                    "timeout "
                                    "button to execute")
                         .arg(target);
                 }

                 const auto &timeoutButtons =
                     getSettings()->timeoutButtons.getValue();
                 if (timeoutButtons.size() < buttonNum || 0 >= buttonNum)
                 {
                     return QString("Invalid argument for execModeratorAction: "
                                    "%1. Integer out of usable range: [1, %2]")
                         .arg(buttonNum, timeoutButtons.size() - 1);
                 }
                 const auto &button = timeoutButtons.at(buttonNum - 1);
                 msg = QString("/timeout %1 %2")
                           .arg(this->userName_)
                           .arg(calculateTimeoutDuration(button));
             }
             this->channel_->sendMessage(msg);
             return "";
         }},

        // these actions make no sense in the context of a usercard, so they aren't implemented
        {"reject", nullptr},
        {"accept", nullptr},
        {"openTab", nullptr},
        {"search", nullptr},
    };

    this->shortcuts_ = getApp()->hotkeys->shortcutsForCategory(
        HotkeyCategory::PopupWindow, actions, this);

    auto layout = LayoutCreator<QWidget>(this->getLayoutContainer())
                      .setLayoutType<QVBoxLayout>();

    // first line
    auto head = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        // avatar
        auto avatar =
            head.emplace<Button>(nullptr).assign(&this->ui_.avatarButton);
        avatar->setScaleIndependantSize(100, 100);
        avatar->setDim(Button::Dim::None);
        QObject::connect(
            avatar.getElement(), &Button::clicked,
            [this](Qt::MouseButton button) {
                switch (button)
                {
                    case Qt::LeftButton: {
                        QDesktopServices::openUrl(QUrl(
                            "https://twitch.tv/" + this->userName_.toLower()));
                    }
                    break;

                    case Qt::RightButton: {
                        // don't raise open context menu if there's no avatar (probably in cases when invalid user's usercard was opened)
                        if (this->avatarUrl_.isEmpty())
                        {
                            return;
                        }

                        static QMenu *previousMenu = nullptr;
                        if (previousMenu != nullptr)
                        {
                            previousMenu->deleteLater();
                            previousMenu = nullptr;
                        }

                        auto menu = new QMenu;
                        previousMenu = menu;

                        auto avatarUrl = this->avatarUrl_;

                        // add context menu actions
                        menu->addAction("Open avatar in browser", [avatarUrl] {
                            QDesktopServices::openUrl(QUrl(avatarUrl));
                        });

                        menu->addAction("Copy avatar link", [avatarUrl] {
                            crossPlatformCopy(avatarUrl);
                        });

                        // we need to assign login name for msvc compilation
                        auto loginName = this->userName_.toLower();
                        menu->addAction(
                            "Open channel in a new popup window", this,
                            [loginName] {
                                auto app = getApp();
                                auto &window = app->windows->createWindow(
                                    WindowType::Popup, true);
                                auto split = window.getNotebook()
                                                 .getOrAddSelectedPage()
                                                 ->appendNewSplit(false);
                                split->setChannel(app->twitch2->getOrAddChannel(
                                    loginName.toLower()));
                            });

                        menu->popup(QCursor::pos());
                        menu->raise();
                    }
                    break;

                    default:;
                }
            });

        auto vbox = head.emplace<QVBoxLayout>();
        {
            // items on the right
            {
                auto box = vbox.emplace<QHBoxLayout>()
                               .withoutMargin()
                               .withoutSpacing();

                this->ui_.nameLabel = addCopyableLabel(box, "Copy name");
                this->ui_.nameLabel->setFontStyle(FontStyle::UiMediumBold);
                box->addSpacing(5);
                box->addStretch(1);

                this->ui_.localizedNameLabel =
                    addCopyableLabel(box, "Copy localized name",
                                     &this->ui_.localizedNameCopyButton);
                this->ui_.localizedNameLabel->setFontStyle(
                    FontStyle::UiMediumBold);
                box->addSpacing(5);
                box->addStretch(1);

                auto palette = QPalette();
                palette.setColor(QPalette::WindowText, QColor("#aaa"));
                this->ui_.userIDLabel = addCopyableLabel(box, "Copy ID");
                this->ui_.userIDLabel->setPalette(palette);

                this->ui_.localizedNameLabel->setVisible(false);
                this->ui_.localizedNameCopyButton->setVisible(false);
            }

            // items on the left
            vbox.emplace<Label>(TEXT_VIEWS.arg(""))
                .assign(&this->ui_.viewCountLabel);
            vbox.emplace<Label>(TEXT_FOLLOWERS.arg(""))
                .assign(&this->ui_.followerCountLabel);
            vbox.emplace<Label>(TEXT_CREATED.arg(""))
                .assign(&this->ui_.createdDateLabel);
            vbox.emplace<Label>("").assign(&this->ui_.followageLabel);
            vbox.emplace<Label>("").assign(&this->ui_.subageLabel);
        }
    }

    layout.emplace<Line>(false);

    // second line
    auto user = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        user->addStretch(1);

        user.emplace<QCheckBox>("Block").assign(&this->ui_.block);
        user.emplace<QCheckBox>("Ignore highlights")
            .assign(&this->ui_.ignoreHighlights);
        auto usercard = user.emplace<EffectLabel2>(this);
        usercard->getLabel().setText("Usercard");

        auto checkAfk = user.emplace<EffectLabel2>(this);
        checkAfk->getLabel().setText("Check AFK");

        auto mod = user.emplace<Button>(this);
        mod->setPixmap(getResources().buttons.mod);
        mod->setScaleIndependantSize(30, 30);
        auto unmod = user.emplace<Button>(this);
        unmod->setPixmap(getResources().buttons.unmod);
        unmod->setScaleIndependantSize(30, 30);
        auto vip = user.emplace<Button>(this);
        vip->setPixmap(getResources().buttons.vip);
        vip->setScaleIndependantSize(30, 30);
        auto unvip = user.emplace<Button>(this);
        unvip->setPixmap(getResources().buttons.unvip);
        unvip->setScaleIndependantSize(30, 30);

        user->addStretch(1);

        QObject::connect(usercard.getElement(), &Button::leftClicked, [this] {
            QDesktopServices::openUrl("https://www.twitch.tv/popout/" +
                                      this->channel_->getName() +
                                      "/viewercard/" + this->userName_);
        });

        QObject::connect(mod.getElement(), &Button::leftClicked, [this] {
            this->channel_->sendMessage("/mod " + this->userName_);
        });
        QObject::connect(unmod.getElement(), &Button::leftClicked, [this] {
            this->channel_->sendMessage("/unmod " + this->userName_);
        });
        QObject::connect(vip.getElement(), &Button::leftClicked, [this] {
            this->channel_->sendMessage("/vip " + this->userName_);
        });
        QObject::connect(unvip.getElement(), &Button::leftClicked, [this] {
            this->channel_->sendMessage("/unvip " + this->userName_);
        });

        QObject::connect(checkAfk.getElement(), &Button::leftClicked, [this] {
            bool ok = this->checkAfkRateLimiter_.try_lock();
            if (!ok)
            {
                return;
            }
            //checkAfk->getLabel().setText("Please wait...");
            auto url = QUrl("https://supinic.com/api/bot/afk/check");
            auto query = QUrlQuery();
            query.addQueryItem("username", this->userName_);

            url.setQuery(query);
            NetworkRequest(url)
                .onSuccess([this](NetworkResult result) -> Outcome {
                    this->checkAfkRateLimiter_.unlock();
                    auto json = result.parseJson();
                    auto afk = json.value("data").toObject().value("status");
                    if (afk.isNull())
                    {
                        // user isn't afk
                        auto builder = MessageBuilder(
                            systemMessage,
                            QString("User %1 is not AFK").arg(this->userName_));
                        builder.message().id = this->userId_;
                        builder.message().loginName = this->userName_;
                        builder.message().displayName = this->userName_;

                        this->channel_->addMessage(builder.release());
                    }
                    else
                    {
                        auto status = afk.toObject();
                        if (status.value("twitchID").isNull())
                        {
                            auto builder = MessageBuilder(
                                systemMessage,
                                QString("User %1 is not known to Supibot on "
                                        "Twitch but they are AFK (%2) from "
                                        "another platform with the message %3")
                                    .arg(this->userName_)
                                    .arg(status.value("status").toString())
                                    .arg(status.value("text").toString()));
                            builder.message().id = this->userId_;
                            builder.message().loginName = this->userName_;
                            builder.message().displayName = this->userName_;

                            this->channel_->addMessage(builder.release());
                            return Success;
                        }
                        auto builder = MessageBuilder(
                            systemMessage,
                            QString("User %1 is AFK (%2): %3")
                                .arg(this->userName_)
                                .arg(status.value("status").toString())
                                .arg(status.value("text").toString()));
                        builder.message().id = this->userId_;
                        builder.message().loginName = this->userName_;
                        builder.message().displayName = this->userName_;

                        this->channel_->addMessage(builder.release());
                    }
                    return Success;
                })
                .onError([this](NetworkResult result) {
                    this->checkAfkRateLimiter_.unlock();
                    auto builder = MessageBuilder(
                        systemMessage,
                        QString("Failed to check AFK status for user %1: %2")
                            .arg(this->userName_)
                            .arg(result.status()));
                    this->channel_->addMessage(builder.release());
                })
                .execute();
        });

        // userstate
        this->userStateChanged_.connect([this, mod, unmod, vip,
                                         unvip]() mutable {
            TwitchChannel *twitchChannel =
                dynamic_cast<TwitchChannel *>(this->channel_.get());

            bool visibilityModButtons = false;

            if (twitchChannel)
            {
                bool isMyself =
                    QString::compare(
                        getApp()->accounts->twitch.getCurrent()->getUserName(),
                        this->userName_, Qt::CaseInsensitive) == 0;

                visibilityModButtons =
                    twitchChannel->isBroadcaster() && !isMyself;
            }
            mod->setVisible(visibilityModButtons);
            unmod->setVisible(visibilityModButtons);
            vip->setVisible(visibilityModButtons);
            unvip->setVisible(visibilityModButtons);
        });
    }

    auto lineMod = layout.emplace<Line>(false);

    // third line
    auto moderation = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        auto timeout = moderation.emplace<TimeoutWidget>();

        this->userStateChanged_.connect([this, lineMod, timeout]() mutable {
            TwitchChannel *twitchChannel =
                dynamic_cast<TwitchChannel *>(this->channel_.get());

            bool hasModRights =
                twitchChannel ? twitchChannel->hasModRights() : false;
            lineMod->setVisible(hasModRights);
            timeout->setVisible(hasModRights);
        });

        timeout->buttonClicked.connect([this](auto item) {
            TimeoutWidget::Action action;
            int arg;
            std::tie(action, arg) = item;

            switch (action)
            {
                case TimeoutWidget::Ban: {
                    if (this->channel_)
                    {
                        this->channel_->sendMessage("/ban " + this->userName_);
                    }
                }
                break;
                case TimeoutWidget::Unban: {
                    if (this->channel_)
                    {
                        this->channel_->sendMessage("/unban " +
                                                    this->userName_);
                    }
                }
                break;
                case TimeoutWidget::Timeout: {
                    if (this->channel_)
                    {
                        this->channel_->sendMessage("/timeout " +
                                                    this->userName_ + " " +
                                                    QString::number(arg));
                    }
                }
                break;
            }
        });
    }

    layout.emplace<Line>(false);

    // fourth line (last messages)
    auto logs = layout.emplace<QVBoxLayout>().withoutMargin();
    {
        this->ui_.noMessagesLabel = new Label("No recent messages");
        this->ui_.noMessagesLabel->setVisible(false);

        this->ui_.latestMessages = new ChannelView(this);
        this->ui_.latestMessages->setMinimumSize(400, 275);
        this->ui_.latestMessages->setSizePolicy(QSizePolicy::Expanding,
                                                QSizePolicy::Expanding);

        logs->addWidget(this->ui_.noMessagesLabel);
        logs->addWidget(this->ui_.latestMessages);
        logs->setAlignment(this->ui_.noMessagesLabel, Qt::AlignHCenter);
    }

    this->installEvents();
    this->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Policy::Ignored);

    this->dragTimer_.callOnTimeout(
        [this, hack = std::weak_ptr<bool>(this->hack_)] {
            if (!hack.lock())
            {
                // Ensure this timer is never called after the object has been destroyed
                return;
            }
            if (!this->isMoving_)
            {
                return;
            }

            this->move(this->requestedDragPos_);
        });
}

void UserInfoPopup::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    for (auto &&child : this->findChildren<QCheckBox *>())
    {
        child->setFont(getFonts()->getFont(FontStyle::UiMedium, this->scale()));
    }
}

void UserInfoPopup::scaleChangedEvent(float /*scale*/)
{
    themeChangedEvent();

    QTimer::singleShot(20, this, [this] {
        auto geo = this->geometry();
        geo.setWidth(10);
        geo.setHeight(10);

        this->setGeometry(geo);
    });
}

void UserInfoPopup::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        this->dragTimer_.start(std::chrono::milliseconds(17));
        this->startPosDrag_ = event->pos();
        this->movingRelativePos = event->localPos();
    }
}

void UserInfoPopup::mouseReleaseEvent(QMouseEvent *event)
{
    this->dragTimer_.stop();
    this->isMoving_ = false;
}

void UserInfoPopup::mouseMoveEvent(QMouseEvent *event)
{
    // Drag the window by the amount changed from inital position
    // Note that we provide a few *units* of deadzone so people don't
    // start dragging the window if they are slow at clicking.
    auto movePos = event->pos() - this->startPosDrag_;
    if (this->isMoving_ || movePos.manhattanLength() > 10.0)
    {
        this->requestedDragPos_ =
            (event->screenPos() - this->movingRelativePos).toPoint();
        this->isMoving_ = true;
    }
}

void UserInfoPopup::installEvents()
{
    std::shared_ptr<bool> ignoreNext = std::make_shared<bool>(false);

    // block
    QObject::connect(
        this->ui_.block, &QCheckBox::stateChanged,
        [this](int newState) mutable {
            auto currentUser = getApp()->accounts->twitch.getCurrent();

            const auto reenableBlockCheckbox = [this] {
                this->ui_.block->setEnabled(true);
            };

            if (!this->ui_.block->isEnabled())
            {
                reenableBlockCheckbox();
                return;
            }

            switch (newState)
            {
                case Qt::CheckState::Unchecked: {
                    this->ui_.block->setEnabled(false);

                    getApp()->accounts->twitch.getCurrent()->unblockUser(
                        this->userId_,
                        [this, reenableBlockCheckbox, currentUser] {
                            this->channel_->addMessage(makeSystemMessage(
                                QString("You successfully unblocked user %1")
                                    .arg(this->userName_)));
                            reenableBlockCheckbox();
                        },
                        [this, reenableBlockCheckbox] {
                            this->channel_->addMessage(makeSystemMessage(
                                QString(
                                    "User %1 couldn't be unblocked, an unknown "
                                    "error occurred!")
                                    .arg(this->userName_)));
                            reenableBlockCheckbox();
                        });
                }
                break;

                case Qt::CheckState::PartiallyChecked: {
                    // We deliberately ignore this state
                }
                break;

                case Qt::CheckState::Checked: {
                    this->ui_.block->setEnabled(false);

                    getApp()->accounts->twitch.getCurrent()->blockUser(
                        this->userId_,
                        [this, reenableBlockCheckbox, currentUser] {
                            this->channel_->addMessage(makeSystemMessage(
                                QString("You successfully blocked user %1")
                                    .arg(this->userName_)));
                            reenableBlockCheckbox();
                        },
                        [this, reenableBlockCheckbox] {
                            this->channel_->addMessage(makeSystemMessage(
                                QString(
                                    "User %1 couldn't be blocked, an unknown "
                                    "error occurred!")
                                    .arg(this->userName_)));
                            reenableBlockCheckbox();
                        });
                }
                break;
            }
        });

    // ignore highlights
    QObject::connect(
        this->ui_.ignoreHighlights, &QCheckBox::clicked,
        [this](bool checked) mutable {
            this->ui_.ignoreHighlights->setEnabled(false);

            if (checked)
            {
                getSettings()->blacklistedUsers.insert(
                    HighlightBlacklistUser{this->userName_, false});
                this->ui_.ignoreHighlights->setEnabled(true);
            }
            else
            {
                const auto &vector = getSettings()->blacklistedUsers.raw();

                for (int i = 0; i < vector.size(); i++)
                {
                    if (this->userName_ == vector[i].getPattern())
                    {
                        getSettings()->blacklistedUsers.removeAt(i);
                        i--;
                    }
                }
                if (getSettings()->isBlacklistedUser(this->userName_))
                {
                    this->ui_.ignoreHighlights->setToolTip(
                        "Name matched by regex");
                }
                else
                {
                    this->ui_.ignoreHighlights->setEnabled(true);
                }
            }
        });
}

void UserInfoPopup::setData(const QString &name, const ChannelPtr &channel)
{
    this->userName_ = name;
    this->channel_ = channel;
    this->setWindowTitle(TEXT_TITLE.arg(name, channel->getName()));

    this->ui_.nameLabel->setText(name);
    this->ui_.nameLabel->setProperty("copy-text", name);

    this->updateUserData();

    this->userStateChanged_.invoke();

    this->updateLatestMessages();
    QTimer::singleShot(1, this, [this] {
        this->setStayInScreenRect(true);
    });
}

void UserInfoPopup::updateLatestMessages()
{
    auto filteredChannel = filterMessages(this->userName_, this->channel_);
    this->ui_.latestMessages->setChannel(filteredChannel);
    this->ui_.latestMessages->setSourceChannel(this->channel_);

    const bool hasMessages = filteredChannel->hasMessages();
    this->ui_.latestMessages->setVisible(hasMessages);
    this->ui_.noMessagesLabel->setVisible(!hasMessages);

    // shrink dialog in case ChannelView goes from visible to hidden
    this->adjustSize();

    this->refreshConnection_ =
        std::make_unique<pajlada::Signals::ScopedConnection>(
            this->channel_->messageAppended.connect([this, hasMessages](
                                                        auto message, auto) {
                if (!checkMessageUserName(this->userName_, message))
                    return;

                if (hasMessages)
                {
                    // display message in ChannelView
                    this->ui_.latestMessages->channel()->addMessage(message);
                }
                else
                {
                    // The ChannelView is currently hidden, so manually refresh
                    // and display the latest messages
                    this->updateLatestMessages();
                }
            }));
}

void UserInfoPopup::updateUserData()
{
    std::weak_ptr<bool> hack = this->hack_;
    auto currentUser = getApp()->accounts->twitch.getCurrent();

    const auto onUserFetchFailed = [this, hack] {
        if (!hack.lock())
        {
            return;
        }

        // this can occur when the account doesn't exist.
        this->ui_.followerCountLabel->setText(
            TEXT_FOLLOWERS.arg(TEXT_UNAVAILABLE));
        this->ui_.viewCountLabel->setText(TEXT_VIEWS.arg(TEXT_UNAVAILABLE));
        this->ui_.createdDateLabel->setText(TEXT_CREATED.arg(TEXT_UNAVAILABLE));

        this->ui_.nameLabel->setText(this->userName_);

        this->ui_.userIDLabel->setText(QString("ID ") +
                                       QString(TEXT_UNAVAILABLE));
        this->ui_.userIDLabel->setProperty("copy-text",
                                           QString(TEXT_UNAVAILABLE));
    };
    const auto onUserFetched = [this, hack,
                                currentUser](const HelixUser &user) {
        if (!hack.lock())
        {
            return;
        }

        this->userId_ = user.id;
        this->avatarUrl_ = user.profileImageUrl;

        // copyable button for login name of users with a localized username
        if (user.displayName.toLower() != user.login)
        {
            this->ui_.localizedNameLabel->setText(user.displayName);
            this->ui_.localizedNameLabel->setProperty("copy-text",
                                                      user.displayName);
            this->ui_.localizedNameLabel->setVisible(true);
            this->ui_.localizedNameCopyButton->setVisible(true);
        }
        else
        {
            this->ui_.nameLabel->setText(user.displayName);
            this->ui_.nameLabel->setProperty("copy-text", user.displayName);
        }

        this->setWindowTitle(
            TEXT_TITLE.arg(user.displayName, this->channel_->getName()));
        this->ui_.viewCountLabel->setText(
            TEXT_VIEWS.arg(localizeNumbers(user.viewCount)));
        this->ui_.createdDateLabel->setText(
            TEXT_CREATED.arg(user.createdAt.section("T", 0, 0)));
        this->ui_.userIDLabel->setText(TEXT_USER_ID + user.id);
        this->ui_.userIDLabel->setProperty("copy-text", user.id);

        if (isInStreamerMode() &&
            getSettings()->streamerModeHideUsercardAvatars)
        {
            this->ui_.avatarButton->setPixmap(getResources().streamerMode);
        }
        else
        {
            this->loadAvatar(user.profileImageUrl);
        }

        getHelix()->getUserFollowers(
            user.id,
            [this, hack](const auto &followers) {
                if (!hack.lock())
                {
                    return;
                }
                this->ui_.followerCountLabel->setText(
                    TEXT_FOLLOWERS.arg(localizeNumbers(followers.total)));
            },
            [] {
                // on failure
            });

        // get ignore state
        bool isIgnoring = false;

        if (auto blocks = currentUser->accessBlockedUserIds();
            blocks->find(user.id) != blocks->end())
        {
            isIgnoring = true;
        }

        // get ignoreHighlights state
        bool isIgnoringHighlights = false;
        const auto &vector = getSettings()->blacklistedUsers.raw();
        for (int i = 0; i < vector.size(); i++)
        {
            if (this->userName_ == vector[i].getPattern())
            {
                isIgnoringHighlights = true;
                break;
            }
        }
        if (getSettings()->isBlacklistedUser(this->userName_) &&
            !isIgnoringHighlights)
        {
            this->ui_.ignoreHighlights->setToolTip("Name matched by regex");
        }
        else
        {
            this->ui_.ignoreHighlights->setEnabled(true);
        }
        this->ui_.block->setChecked(isIgnoring);
        this->ui_.block->setEnabled(true);
        this->ui_.ignoreHighlights->setChecked(isIgnoringHighlights);

        // get followage and subage
        getIvr()->getSubage(
            this->userName_, this->channel_->getName(),
            [this, hack](const IvrSubage &subageInfo) {
                if (!hack.lock())
                {
                    return;
                }

                if (!subageInfo.followingSince.isEmpty())
                {
                    QDateTime followedAt = QDateTime::fromString(
                        subageInfo.followingSince, Qt::ISODate);
                    QString followingSince = followedAt.toString("yyyy-MM-dd");
                    this->ui_.followageLabel->setText("❤ Following since " +
                                                      followingSince);
                }

                if (subageInfo.isSubHidden)
                {
                    this->ui_.subageLabel->setText(
                        "Subscription status hidden");
                }
                else if (subageInfo.isSubbed)
                {
                    this->ui_.subageLabel->setText(
                        QString("★ Tier %1 - Subscribed for %2 months")
                            .arg(subageInfo.subTier)
                            .arg(subageInfo.totalSubMonths));
                }
                else if (subageInfo.totalSubMonths)
                {
                    this->ui_.subageLabel->setText(
                        QString("★ Previously subscribed for %1 months")
                            .arg(subageInfo.totalSubMonths));
                }
            },
            [] {});
    };

    getHelix()->getUserByName(this->userName_, onUserFetched,
                              onUserFetchFailed);

    this->ui_.block->setEnabled(false);
    this->ui_.ignoreHighlights->setEnabled(false);
}

void UserInfoPopup::loadAvatar(const QUrl &url)
{
    QNetworkRequest req(url);
    static auto manager = new QNetworkAccessManager();
    auto *reply = manager->get(req);

    QObject::connect(reply, &QNetworkReply::finished, this, [=] {
        if (reply->error() == QNetworkReply::NoError)
        {
            const auto data = reply->readAll();

            // might want to cache the avatar image
            QPixmap avatar;
            avatar.loadFromData(data);
            this->ui_.avatarButton->setPixmap(avatar);
        }
        else
        {
            this->ui_.avatarButton->setPixmap(QPixmap());
        }
    });
}

//
// TimeoutWidget
//
UserInfoPopup::TimeoutWidget::TimeoutWidget()
    : BaseWidget(nullptr)
{
    auto layout = LayoutCreator<TimeoutWidget>(this)
                      .setLayoutType<QHBoxLayout>()
                      .withoutMargin();

    QColor color1(255, 255, 255, 80);
    QColor color2(255, 255, 255, 0);

    int buttonWidth = 40;
    // int buttonWidth = 24;
    int buttonWidth2 = 32;
    int buttonHeight = 32;

    layout->setSpacing(16);

    //auto addButton = [&](Action action, const QString &text,
    //                     const QPixmap &pixmap) {
    //    auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
    //    {
    //        auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
    //        title->addStretch(1);
    //        auto label = title.emplace<Label>(text);
    //        label->setHasOffset(false);
    //        label->setStyleSheet("color: #BBB");
    //        title->addStretch(1);

    //        auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
    //        hbox->setSpacing(0);
    //        {
    //            auto button = hbox.emplace<Button>(nullptr);
    //            button->setPixmap(pixmap);
    //            button->setScaleIndependantSize(buttonHeight, buttonHeight);
    //            button->setBorderColor(QColor(255, 255, 255, 127));

    //            QObject::connect(
    //                button.getElement(), &Button::leftClicked, [this, action] {
    //                    this->buttonClicked.invoke(std::make_pair(action, -1));
    //                });
    //        }
    //    }
    //};

    const auto addLayout = [&](const QString &text) {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
        title->addStretch(1);
        auto label = title.emplace<Label>(text);
        label->setStyleSheet("color: #BBB");
        label->setHasOffset(false);
        title->addStretch(1);

        auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
        hbox->setSpacing(0);
        return hbox;
    };

    const auto addButton = [&](Action action, const QString &title,
                               const QPixmap &pixmap) {
        auto button = addLayout(title).emplace<Button>(nullptr);
        button->setPixmap(pixmap);
        button->setScaleIndependantSize(buttonHeight, buttonHeight);
        button->setBorderColor(QColor(255, 255, 255, 127));

        QObject::connect(
            button.getElement(), &Button::leftClicked, [this, action] {
                this->buttonClicked.invoke(std::make_pair(action, -1));
            });
    };

    auto addTimeouts = [&](const QString &title) {
        auto hbox = addLayout(title);

        for (const auto &item : getSettings()->timeoutButtons.getValue())
        {
            auto a = hbox.emplace<EffectLabel2>();
            a->getLabel().setText(QString::number(item.second) + item.first);

            a->setScaleIndependantSize(buttonWidth, buttonHeight);
            a->setBorderColor(borderColor);

            const auto pair =
                std::make_pair(Action::Timeout, calculateTimeoutDuration(item));

            QObject::connect(a.getElement(), &EffectLabel2::leftClicked,
                             [this, pair] {
                                 this->buttonClicked.invoke(pair);
                             });

            //auto addTimeouts = [&](const QString &title_,
            //                       const std::vector<std::pair<QString, int>> &items) {
            //    auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
            //    {
            //        auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            //        title->addStretch(1);
            //        auto label = title.emplace<Label>(title_);
            //        label->setStyleSheet("color: #BBB");
            //        label->setHasOffset(false);
            //        title->addStretch(1);

            //        auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            //        hbox->setSpacing(0);

            //        for (const auto &item : items)
            //        {
            //            auto a = hbox.emplace<EffectLabel2>();
            //            a->getLabel().setText(std::get<0>(item));

            //            if (std::get<0>(item).length() > 1)
            //            {
            //                a->setScaleIndependantSize(buttonWidth2, buttonHeight);
            //            }
            //            else
            //            {
            //                a->setScaleIndependantSize(buttonWidth, buttonHeight);
            //            }
            //            a->setBorderColor(color1);

            //            QObject::connect(a.getElement(), &EffectLabel2::leftClicked,
            //                             [this, timeout = std::get<1>(item)] {
            //                                 this->buttonClicked.invoke(std::make_pair(
            //                                     Action::Timeout, timeout));
            //                             });
            //        }
        }
    };

    addButton(Unban, "Unban", getResources().buttons.unban);
    addTimeouts("Timeouts");
    addButton(Ban, "Ban", getResources().buttons.ban);
}

void UserInfoPopup::TimeoutWidget::paintEvent(QPaintEvent *)
{
    //    QPainter painter(this);

    //    painter.setPen(QColor(255, 255, 255, 63));

    //    painter.drawLine(0, this->height() / 2, this->width(), this->height()
    //    / 2);
}

}  // namespace chatterino
