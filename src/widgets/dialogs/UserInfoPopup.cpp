#include "UserInfoPopup.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "providers/twitch/PartialTwitchUser.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "util/PostToThread.hpp"
#include "widgets/Label.hpp"
#include "widgets/dialogs/LogsPopup.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/Line.hpp"

#include <QCheckBox>
#include <QDesktopServices>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#define TEXT_FOLLOWERS "Followers: "
#define TEXT_VIEWS "Views: "
#define TEXT_CREATED "Created: "
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
}  // namespace

UserInfoPopup::UserInfoPopup()
    : BaseWindow({BaseWindow::Frameless, BaseWindow::FramelessDraggable})
    , hack_(new bool)
{
    this->setStayInScreenRect(true);

#ifdef Q_OS_LINUX
    this->setWindowFlag(Qt::Popup);
#endif

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

        // items on the right
        auto vbox = head.emplace<QVBoxLayout>();
        {
            {
                auto box = vbox.emplace<QHBoxLayout>()
                               .withoutMargin()
                               .withoutSpacing();
                this->ui_.nameLabel = addCopyableLabel(box);
                this->ui_.nameLabel->setFontStyle(FontStyle::UiMediumBold);
                box->addStretch(1);
                this->ui_.userIDLabel = addCopyableLabel(box);
                auto palette = QPalette();
                palette.setColor(QPalette::WindowText, QColor("#aaa"));
                this->ui_.userIDLabel->setPalette(palette);
            }

            vbox.emplace<Label>(TEXT_VIEWS).assign(&this->ui_.viewCountLabel);
            vbox.emplace<Label>(TEXT_FOLLOWERS)
                .assign(&this->ui_.followerCountLabel);
            vbox.emplace<Label>(TEXT_CREATED)
                .assign(&this->ui_.createdDateLabel);
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
        auto viewLogs = user.emplace<EffectLabel2>(this);
        viewLogs->getLabel().setText("Online logs");
        auto usercard = user.emplace<EffectLabel2>(this);
        usercard->getLabel().setText("Usercard");

        auto mod = user.emplace<Button>(this);
        mod->setPixmap(getResources().buttons.mod);
        mod->setScaleIndependantSize(30, 30);
        auto unmod = user.emplace<Button>(this);
        unmod->setPixmap(getResources().buttons.unmod);
        unmod->setScaleIndependantSize(30, 30);

        user->addStretch(1);

        QObject::connect(viewLogs.getElement(), &Button::leftClicked, [this] {
            auto logs = new LogsPopup();
            logs->setChannel(this->channel_);
            logs->setTargetUserName(this->userName_);
            logs->getLogs();
            logs->setAttribute(Qt::WA_DeleteOnClose);
            logs->show();
        });

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

        // userstate
        this->userStateChanged_.connect([this, mod, unmod]() mutable {
            TwitchChannel *twitchChannel =
                dynamic_cast<TwitchChannel *>(this->channel_.get());

            bool visibilityModButtons = false;

            if (twitchChannel)
            {
                qDebug() << this->userName_;

                bool isMyself =
                    QString::compare(
                        getApp()->accounts->twitch.getCurrent()->getUserName(),
                        this->userName_, Qt::CaseInsensitive) == 0;

                visibilityModButtons =
                    twitchChannel->isBroadcaster() && !isMyself;
            }
            mod->setVisible(visibilityModButtons);
            unmod->setVisible(visibilityModButtons);
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

    this->installEvents();

    this->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Policy::Ignored);
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
        this->ui_.follow, &QCheckBox::stateChanged, [this](int) mutable {
            auto currentUser = getApp()->accounts->twitch.getCurrent();

            QUrl requestUrl("https://api.twitch.tv/kraken/users/" +
                            currentUser->getUserId() + "/follows/channels/" +
                            this->userId_);

            const auto reenableFollowCheckbox = [this] {
                this->ui_.follow->setEnabled(true);  //
            };

            this->ui_.follow->setEnabled(false);
            if (this->ui_.follow->isChecked())
            {
                currentUser->followUser(this->userId_, reenableFollowCheckbox);
            }
            else
            {
                currentUser->unfollowUser(this->userId_,
                                          reenableFollowCheckbox);
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

    this->ui_.nameLabel->setText(name);
    this->ui_.nameLabel->setProperty("copy-text", name);

    this->updateUserData();

    this->userStateChanged_.invoke();
}

void UserInfoPopup::updateUserData()
{
    std::weak_ptr<bool> hack = this->hack_;

    const auto onIdFetchFailed = [this]() {
        // this can occur when the account doesn't exist.
        this->ui_.followerCountLabel->setText(TEXT_FOLLOWERS +
                                              QString(TEXT_UNAVAILABLE));
        this->ui_.viewCountLabel->setText(TEXT_VIEWS +
                                          QString(TEXT_UNAVAILABLE));
        this->ui_.createdDateLabel->setText(TEXT_CREATED +
                                            QString(TEXT_UNAVAILABLE));

        this->ui_.nameLabel->setText(this->userName_);

        this->ui_.userIDLabel->setText(QString("ID") +
                                       QString(TEXT_UNAVAILABLE));
        this->ui_.userIDLabel->setProperty("copy-text",
                                           QString(TEXT_UNAVAILABLE));
    };
    const auto onIdFetched = [this, hack](QString id) {
        auto currentUser = getApp()->accounts->twitch.getCurrent();

        this->userId_ = id;

        this->ui_.userIDLabel->setText(TEXT_USER_ID + id);
        this->ui_.userIDLabel->setProperty("copy-text", id);
        // don't wait for the request to complete, just put the user id in the card
        // right away

        QString url("https://api.twitch.tv/kraken/channels/" + id);

        NetworkRequest::twitchRequest(url)
            .caller(this)
            .onSuccess([this](auto result) -> Outcome {
                auto obj = result.parseJson();
                this->ui_.followerCountLabel->setText(
                    TEXT_FOLLOWERS +
                    QString::number(obj.value("followers").toInt()));
                this->ui_.viewCountLabel->setText(
                    TEXT_VIEWS + QString::number(obj.value("views").toInt()));
                this->ui_.createdDateLabel->setText(
                    TEXT_CREATED +
                    obj.value("created_at").toString().section("T", 0, 0));

                this->loadAvatar(QUrl(obj.value("logo").toString()));

                return Success;
            })
            .execute();

        // get follow state
        currentUser->checkFollow(id, [this, hack](auto result) {
            if (hack.lock())
            {
                if (result != FollowResult_Failed)
                {
                    this->ui_.follow->setEnabled(true);
                    this->ui_.follow->setChecked(result ==
                                                 FollowResult_Following);
                }
            }
        });

        // get ignore state
        bool isIgnoring = false;
        for (const auto &ignoredUser : currentUser->getIgnores())
        {
            if (id == ignoredUser.id)
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
    };

    PartialTwitchUser::byName(this->userName_)
        .getId(onIdFetched, onIdFetchFailed, this);

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

    int buttonWidth = 24;
    int buttonWidth2 = 32;
    int buttonHeight = 32;

    layout->setSpacing(16);

    auto addButton = [&](Action action, const QString &text,
                         const QPixmap &pixmap) {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            title->addStretch(1);
            auto label = title.emplace<Label>(text);
            label->setHasOffset(false);
            label->setStyleSheet("color: #BBB");
            title->addStretch(1);

            auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            hbox->setSpacing(0);
            {
                auto button = hbox.emplace<Button>(nullptr);
                button->setPixmap(pixmap);
                button->setScaleIndependantSize(buttonHeight, buttonHeight);
                button->setBorderColor(QColor(255, 255, 255, 127));

                QObject::connect(
                    button.getElement(), &Button::leftClicked, [this, action] {
                        this->buttonClicked.invoke(std::make_pair(action, -1));
                    });
            }
        }
    };

    auto addTimeouts = [&](const QString &title_,
                           const std::vector<std::pair<QString, int>> &items) {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            title->addStretch(1);
            auto label = title.emplace<Label>(title_);
            label->setStyleSheet("color: #BBB");
            label->setHasOffset(false);
            title->addStretch(1);

            auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            hbox->setSpacing(0);

            for (const auto &item : items)
            {
                auto a = hbox.emplace<EffectLabel2>();
                a->getLabel().setText(std::get<0>(item));

                if (std::get<0>(item).length() > 1)
                {
                    a->setScaleIndependantSize(buttonWidth2, buttonHeight);
                }
                else
                {
                    a->setScaleIndependantSize(buttonWidth, buttonHeight);
                }
                a->setBorderColor(color1);

                QObject::connect(a.getElement(), &EffectLabel2::leftClicked,
                                 [this, timeout = std::get<1>(item)] {
                                     this->buttonClicked.invoke(std::make_pair(
                                         Action::Timeout, timeout));
                                 });
            }
        }
    };

    addButton(Unban, "unban", getResources().buttons.unban);

    addTimeouts("sec", {{"1", 1}});
    addTimeouts("min", {
                           {"1", 1 * 60},
                           {"5", 5 * 60},
                           {"10", 10 * 60},
                       });
    addTimeouts("hour", {
                            {"1", 1 * 60 * 60},
                            {"4", 4 * 60 * 60},
                        });
    addTimeouts("days", {
                            {"1", 1 * 60 * 60 * 24},
                            {"3", 3 * 60 * 60 * 24},
                        });
    addTimeouts("weeks", {
                             {"1", 1 * 60 * 60 * 24 * 7},
                             {"2", 2 * 60 * 60 * 24 * 7},
                         });

    addButton(Ban, "ban", getResources().buttons.ban);
}

void UserInfoPopup::TimeoutWidget::paintEvent(QPaintEvent *)
{
    //    QPainter painter(this);

    //    painter.setPen(QColor(255, 255, 255, 63));

    //    painter.drawLine(0, this->height() / 2, this->width(), this->height()
    //    / 2);
}

}  // namespace chatterino
