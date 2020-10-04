#include "UserInfoPopup.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "messages/Message.hpp"
#include "providers/IvrApi.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/api/Kraken.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "util/PostToThread.hpp"
#include "util/Shortcut.hpp"
#include "util/StreamerMode.hpp"
#include "widgets/Label.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/Line.hpp"

#include <QCheckBox>
#include <QDesktopServices>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>

const QString TEXT_VIEWS("Views: %1");
const QString TEXT_FOLLOWERS("Followers: %1");
const QString TEXT_CREATED("Created: %1");
const QString TEXT_TITLE("%1's Usercard");
#define TEXT_USER_ID "ID: "
#define TEXT_UNAVAILABLE "(not available)"

namespace chatterino {
namespace {
    Label *addCopyableLabel(LayoutCreator<QHBoxLayout> box)
    {
        auto label = box.emplace<Label>();
        auto button = box.emplace<Button>();
        button->setPixmap(getResources().buttons.copyDark);
        button->setScaleIndependantSize(18, 18);
        button->setDim(Button::Dim::Lots);
        QObject::connect(
            button.getElement(), &Button::leftClicked,
            [label = label.getElement()] {
                auto copyText = label->property("copy-text").toString();

                qApp->clipboard()->setText(copyText.isEmpty() ? label->getText()
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

UserInfoPopup::UserInfoPopup(bool closeAutomatically)
    : BaseWindow(
          closeAutomatically
              ? FlagsEnum<BaseWindow::Flags>{BaseWindow::EnableCustomFrame,
                                             BaseWindow::Frameless,
                                             BaseWindow::FramelessDraggable}
              : BaseWindow::EnableCustomFrame)
    , hack_(new bool)
{
    this->setWindowTitle("Usercard");
    this->setStayInScreenRect(true);

    if (closeAutomatically)
        this->setActionOnFocusLoss(BaseWindow::Delete);
    else
        this->setAttribute(Qt::WA_DeleteOnClose);

    // Close the popup when Escape is pressed
    createWindowShortcut(this, "Escape", [this] { this->deleteLater(); });

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
        QObject::connect(avatar.getElement(), &Button::leftClicked, [this] {
            QDesktopServices::openUrl(
                QUrl("https://twitch.tv/" + this->userName_.toLower()));
        });

        auto vbox = head.emplace<QVBoxLayout>();
        {
            // items on the right
            {
                auto box = vbox.emplace<QHBoxLayout>()
                               .withoutMargin()
                               .withoutSpacing();
                this->ui_.nameLabel = addCopyableLabel(box);
                this->ui_.nameLabel->setFontStyle(FontStyle::UiMediumBold);
                box->addStretch(1);
                auto palette = QPalette();
                palette.setColor(QPalette::WindowText, QColor("#aaa"));
                this->ui_.userIDLabel = addCopyableLabel(box);
                this->ui_.userIDLabel->setPalette(palette);
            }

            // items on the left
            vbox.emplace<Label>(TEXT_VIEWS.arg(""))
                .assign(&this->ui_.viewCountLabel);
            vbox.emplace<Label>(TEXT_FOLLOWERS.arg(""))
                .assign(&this->ui_.followerCountLabel);
            vbox.emplace<Label>(TEXT_CREATED.arg(""))
                .assign(&this->ui_.createdDateLabel);
            vbox.emplace<Line>(true);
            vbox.emplace<Label>("")
                .assign(&this->ui_.followageSubageLabel)
                ->setMinimumSize(this->minimumSizeHint());
        }
    }

    layout.emplace<Line>(false);

    // second line
    auto user = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        user->addStretch(1);

        user.emplace<QCheckBox>("Follow").assign(&this->ui_.follow);
        user.emplace<QCheckBox>("Ignore").assign(&this->ui_.ignore);
        user.emplace<QCheckBox>("Ignore highlights")
            .assign(&this->ui_.ignoreHighlights);
        auto usercard = user.emplace<EffectLabel2>(this);
        usercard->getLabel().setText("Usercard");
        auto refresh = user.emplace<EffectLabel2>(this);
        refresh->getLabel().setText("Refresh");
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

        QObject::connect(refresh.getElement(), &Button::leftClicked,
                         [this] { this->updateLatestMessages(); });
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
}

// remove once https://github.com/pajlada/signals/pull/10 gets merged
UserInfoPopup::~UserInfoPopup()
{
    this->refreshConnection_.disconnect();
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

void UserInfoPopup::installEvents()
{
    std::weak_ptr<bool> hack = this->hack_;

    // follow
    QObject::connect(
        this->ui_.follow, &QCheckBox::stateChanged,
        [this](int newState) mutable {
            auto currentUser = getApp()->accounts->twitch.getCurrent();

            const auto reenableFollowCheckbox = [this] {
                this->ui_.follow->setEnabled(true);  //
            };

            if (!this->ui_.follow->isEnabled())
            {
                // We received a state update while the checkbox was disabled
                // This can only happen from the "check current follow state" call
                // The state has been updated to properly reflect the users current follow state
                reenableFollowCheckbox();
                return;
            }

            switch (newState)
            {
                case Qt::CheckState::Unchecked: {
                    this->ui_.follow->setEnabled(false);
                    currentUser->unfollowUser(this->userId_,
                                              reenableFollowCheckbox);
                }
                break;

                case Qt::CheckState::PartiallyChecked: {
                    // We deliberately ignore this state
                }
                break;

                case Qt::CheckState::Checked: {
                    this->ui_.follow->setEnabled(false);
                    currentUser->followUser(this->userId_,
                                            reenableFollowCheckbox);
                }
                break;
            }
        });

    std::shared_ptr<bool> ignoreNext = std::make_shared<bool>(false);

    // ignore
    QObject::connect(
        this->ui_.ignore, &QCheckBox::stateChanged,
        [this, ignoreNext, hack](int) mutable {
            if (*ignoreNext)
            {
                *ignoreNext = false;
                return;
            }

            this->ui_.ignore->setEnabled(false);

            auto currentUser = getApp()->accounts->twitch.getCurrent();
            if (this->ui_.ignore->isChecked())
            {
                currentUser->ignoreByID(
                    this->userId_, this->userName_,
                    [=](auto result, const auto &message) mutable {
                        if (hack.lock())
                        {
                            if (result == IgnoreResult_Failed)
                            {
                                *ignoreNext = true;
                                this->ui_.ignore->setChecked(false);
                            }
                            this->ui_.ignore->setEnabled(true);
                        }
                    });
            }
            else
            {
                currentUser->unignoreByID(
                    this->userId_, this->userName_,
                    [=](auto result, const auto &message) mutable {
                        if (hack.lock())
                        {
                            if (result == UnignoreResult_Failed)
                            {
                                *ignoreNext = true;
                                this->ui_.ignore->setChecked(true);
                            }
                            this->ui_.ignore->setEnabled(true);
                        }
                    });
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
    this->setWindowTitle(TEXT_TITLE.arg(name));

    this->ui_.nameLabel->setText(name);
    this->ui_.nameLabel->setProperty("copy-text", name);

    this->updateUserData();

    this->userStateChanged_.invoke();

    this->updateLatestMessages();
    QTimer::singleShot(1, this, [this] { this->setStayInScreenRect(true); });
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

    this->refreshConnection_
        .disconnect();  // remove once https://github.com/pajlada/signals/pull/10 gets merged

    this->refreshConnection_ = this->channel_->messageAppended.connect(
        [this, hasMessages](auto message, auto) {
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
        });
}

void UserInfoPopup::updateUserData()
{
    this->ui_.follow->setEnabled(false);

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

        this->ui_.userIDLabel->setText(TEXT_USER_ID + user.id);
        this->ui_.userIDLabel->setProperty("copy-text", user.id);

        this->ui_.viewCountLabel->setText(TEXT_VIEWS.arg(user.viewCount));
        getKraken()->getUser(
            user.id,
            [this, hack](const auto &user) {
                if (!hack.lock())
                {
                    return;
                }
                this->ui_.createdDateLabel->setText(
                    TEXT_CREATED.arg(user.createdAt.section("T", 0, 0)));
            },
            [] {
                // failure
            });
        if (isInStreamerMode())
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
                    TEXT_FOLLOWERS.arg(followers.total));
            },
            [] {
                // on failure
            });

        // get follow state
        currentUser->checkFollow(user.id, [this, hack](auto result) {
            if (!hack.lock())
            {
                return;
            }
            if (result != FollowResult_Failed)
            {
                this->ui_.follow->setChecked(result == FollowResult_Following);
                this->ui_.follow->setEnabled(true);
            }
        });

        // get ignore state
        bool isIgnoring = false;
        for (const auto &ignoredUser : currentUser->getIgnores())
        {
            if (user.id == ignoredUser.id)
            {
                isIgnoring = true;
                break;
            }
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
        this->ui_.ignore->setEnabled(true);
        this->ui_.ignore->setChecked(isIgnoring);
        this->ui_.ignoreHighlights->setChecked(isIgnoringHighlights);

        // get followage and subage
        getIvr()->getSubage(
            this->userName_, this->channel_->getName(),
            [this, hack](const IvrSubage &subageInfo) {
                QString labelText;

                if (!subageInfo.followingSince.isEmpty())
                {
                    QDateTime followedAt = QDateTime::fromString(
                        subageInfo.followingSince, Qt::ISODate);
                    QString followingSince = followedAt.toString("yyyy-MM-dd");
                    labelText = "Following since " + followingSince;
                }

                if (subageInfo.isSubHidden)
                {
                    labelText += "\nSubscribtion status hidden";
                }
                if (subageInfo.isSubbed)
                {
                    labelText += QString("\nTier %1 - Subscribed for %2 months")
                                     .arg(subageInfo.subTier)
                                     .arg(subageInfo.totalSubMonths);
                }
                else if (subageInfo.totalSubMonths)
                {
                    labelText +=
                        QString("\nPreviously subscribed for %1 months")
                            .arg(subageInfo.totalSubMonths);
                }

                this->ui_.followageSubageLabel->setText(labelText);
            },
            [] {});
    };

    getHelix()->getUserByName(this->userName_, onUserFetched,
                              onUserFetchFailed);

    this->ui_.follow->setEnabled(false);
    this->ui_.ignore->setEnabled(false);
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

            QObject::connect(
                a.getElement(), &EffectLabel2::leftClicked,
                [this, pair] { this->buttonClicked.invoke(pair); });

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
