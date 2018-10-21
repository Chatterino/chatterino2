#include "UserInfoPopup.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "providers/twitch/PartialTwitchUser.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Resources.hpp"
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

namespace chatterino {

UserInfoPopup::UserInfoPopup()
    : BaseWindow(nullptr, BaseWindow::Flags(BaseWindow::Frameless |
                                            BaseWindow::FramelessDraggable))
    , hack_(new bool)
{
    this->setStayInScreenRect(true);

#ifdef Q_OS_LINUX
    this->setWindowFlag(Qt::Popup);
#endif

    auto app = getApp();

    auto layout =
        LayoutCreator<UserInfoPopup>(this).setLayoutType<QVBoxLayout>();

    // first line
    auto head = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        // avatar
        auto avatar =
            head.emplace<Button>(nullptr).assign(&this->ui_.avatarButton);
        avatar->setScaleIndependantSize(100, 100);
        QObject::connect(avatar.getElement(), &Button::leftClicked, [this] {
            QDesktopServices::openUrl(
                QUrl("https://twitch.tv/" + this->userName_.toLower()));
        });

        // items on the right
        auto vbox = head.emplace<QVBoxLayout>();
        {
            auto name = vbox.emplace<Label>().assign(&this->ui_.nameLabel);

            auto font = name->font();
            font.setBold(true);
            name->setFont(font);
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

        auto mod = user.emplace<Button>(this);
        mod->setPixmap(app->resources->buttons.mod);
        mod->setScaleIndependantSize(30, 30);
        auto unmod = user.emplace<Button>(this);
        unmod->setPixmap(app->resources->buttons.unmod);
        unmod->setScaleIndependantSize(30, 30);

        user->addStretch(1);

        QObject::connect(viewLogs.getElement(), &Button::leftClicked, [this] {
            auto logs = new LogsPopup();
            logs->setInfo(this->channel_, this->userName_);
            logs->setAttribute(Qt::WA_DeleteOnClose);
            logs->show();
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

            if (twitchChannel)
            {
                qDebug() << this->userName_;

                bool isMyself =
                    QString::compare(
                        getApp()->accounts->twitch.getCurrent()->getUserName(),
                        this->userName_, Qt::CaseInsensitive) == 0;

                mod->setVisible(twitchChannel->isBroadcaster() && !isMyself);
                unmod->setVisible(
                    (twitchChannel->isBroadcaster() && !isMyself) ||
                    (twitchChannel->isMod() && isMyself));
            }
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

            if (twitchChannel)
            {
                lineMod->setVisible(twitchChannel->hasModRights());
                timeout->setVisible(twitchChannel->hasModRights());
            }
        });

        timeout->buttonClicked.connect([this](auto item) {
            TimeoutWidget::Action action;
            int arg;
            std::tie(action, arg) = item;

            switch (action)
            {
                case TimeoutWidget::Ban:
                {
                    if (this->channel_)
                    {
                        this->channel_->sendMessage("/ban " + this->userName_);
                    }
                }
                break;
                case TimeoutWidget::Unban:
                {
                    if (this->channel_)
                    {
                        this->channel_->sendMessage("/unban " +
                                                    this->userName_);
                    }
                }
                break;
                case TimeoutWidget::Timeout:
                {
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

    this->setStyleSheet("font-size: 11pt;");

    this->installEvents();
}

void UserInfoPopup::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    this->setStyleSheet("background: #333");
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
                getApp()->highlights->blacklistedUsers.insertItem(
                    HighlightBlacklistUser{this->userName_, false});
                this->ui_.ignoreHighlights->setEnabled(true);
            }
            else
            {
                const auto &vector =
                    getApp()->highlights->blacklistedUsers.getVector();

                for (int i = 0; i < vector.size(); i++)
                {
                    if (this->userName_ == vector[i].getPattern())
                    {
                        getApp()->highlights->blacklistedUsers.removeItem(i);
                        i--;
                    }
                }
                if (getApp()->highlights->blacklistContains(this->userName_))
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

    this->updateUserData();

    this->userStateChanged_.invoke();
}

void UserInfoPopup::updateUserData()
{
    std::weak_ptr<bool> hack = this->hack_;

    const auto onIdFetched = [this, hack](QString id) {
        auto currentUser = getApp()->accounts->twitch.getCurrent();

        this->userId_ = id;

        QString url("https://api.twitch.tv/kraken/channels/" + id);

        auto request = NetworkRequest::twitchRequest(url);
        request.setCaller(this);

        request.onSuccess([this](auto result) -> Outcome {
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
        });

        request.execute();

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
        const auto &vector = getApp()->highlights->blacklistedUsers.getVector();
        for (int i = 0; i < vector.size(); i++)
        {
            if (this->userName_ == vector[i].getPattern())
            {
                isIgnoringHighlights = true;
                break;
            }
        }
        if (getApp()->highlights->blacklistContains(this->userName_) &&
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

    PartialTwitchUser::byName(this->userName_).getId(onIdFetched, this);

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

    addButton(Unban, "unban", getApp()->resources->buttons.unban);

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

    addButton(Ban, "ban", getApp()->resources->buttons.ban);
}

void UserInfoPopup::TimeoutWidget::paintEvent(QPaintEvent *)
{
    //    QPainter painter(this);

    //    painter.setPen(QColor(255, 255, 255, 63));

    //    painter.drawLine(0, this->height() / 2, this->width(), this->height()
    //    / 2);
}

}  // namespace chatterino
