#include "widgets/accountpopup.hpp"

#include "application.hpp"
#include "channel.hpp"
#include "credentials.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "ui_accountpopupform.h"
#include "util/urlfetch.hpp"

#include <QClipboard>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>

namespace chatterino {
namespace widgets {

AccountPopupWidget::AccountPopupWidget(ChannelPtr _channel)
    : BaseWindow()
    , ui(new Ui::AccountPopup)
    , channel(_channel)
{
    auto app = getApp();

    this->ui->setupUi(this);

    this->setStayInScreenRect(true);

    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    this->setWindowFlags(Qt::FramelessWindowHint);

    this->resize(0, 0);

    connect(this, &AccountPopupWidget::refreshButtons, this,
            &AccountPopupWidget::actuallyRefreshButtons, Qt::QueuedConnection);

    app->accounts->Twitch.userChanged.connect([=] {
        auto currentTwitchUser = app->accounts->Twitch.getCurrent();
        if (!currentTwitchUser) {
            // No twitch user set (should never happen)
            return;
        }

        this->loggedInUser.username = currentTwitchUser->getUserName();
        this->loggedInUser.userID = currentTwitchUser->getUserId();

        this->loggedInUser.refreshUserType(this->channel, true);
    });

    for (auto button : this->ui->profileLayout->findChildren<QPushButton *>()) {
        button->setFocusProxy(this);
    }
    for (auto button : this->ui->userLayout->findChildren<QPushButton *>()) {
        button->setFocusProxy(this);
    }
    for (auto button : this->ui->modLayout->findChildren<QPushButton *>()) {
        button->setFocusProxy(this);
    }
    for (auto button : this->ui->ownerLayout->findChildren<QPushButton *>()) {
        button->setFocusProxy(this);
    }

    this->timeout(this->ui->purge, 1);
    this->timeout(this->ui->min1, 60);
    this->timeout(this->ui->min10, 600);
    this->timeout(this->ui->hour1, 3600);
    this->timeout(this->ui->hour24, 86400);

    this->sendCommand(this->ui->ban, "/ban ");
    this->sendCommand(this->ui->unBan, "/unban ");
    this->sendCommand(this->ui->mod, "/mod ");
    this->sendCommand(this->ui->unMod, "/unmod ");

    QObject::connect(this->ui->profile, &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://twitch.tv/" + this->popupWidgetUser.username));
    });

    QObject::connect(this->ui->sendMessage, &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(
            QUrl("https://www.twitch.tv/message/compose?to=" + this->ui->lblUsername->text()));
    });

    QObject::connect(this->ui->copy, &QPushButton::clicked, this,
                     [=]() { QApplication::clipboard()->setText(this->ui->lblUsername->text()); });

    QObject::connect(this->ui->follow, &QPushButton::clicked, this, [=]() {
        debug::Log("Attempt to toggle follow user {}({}) as user {}({})",
                   this->popupWidgetUser.username, this->popupWidgetUser.userID,
                   this->loggedInUser.username, this->loggedInUser.userID);

        QUrl requestUrl("https://api.twitch.tv/kraken/users/" + this->loggedInUser.userID +
                        "/follows/channels/" + this->popupWidgetUser.userID);

        this->ui->follow->setEnabled(false);
        if (!this->relationship.following) {
            util::twitch::put(requestUrl, [this](QJsonObject obj) {
                qDebug() << "follows channel: " << obj;
                this->relationship.following = true;
                emit refreshButtons();
            });
        } else {
            util::twitch::sendDelete(requestUrl, [this] {
                this->relationship.following = false;
                emit refreshButtons();
            });
        }
    });

    QObject::connect(this->ui->ignore, &QPushButton::clicked, this, [=]() {
        QUrl requestUrl("https://api.twitch.tv/kraken/users/" + this->loggedInUser.userID +
                        "/blocks/" + this->popupWidgetUser.userID);

        if (!this->relationship.ignoring) {
            util::twitch::put(requestUrl, [this](auto) {
                this->relationship.ignoring = true;  //
            });
        } else {
            util::twitch::sendDelete(requestUrl, [this] {
                this->relationship.ignoring = false;  //
            });
        }
    });

    QObject::connect(this->ui->disableHighlights, &QPushButton::clicked, this, [=]() {
        QString str = app->settings->highlightUserBlacklist;
        str.append(this->ui->lblUsername->text() + "\n");
        app->settings->highlightUserBlacklist = str;
        this->ui->disableHighlights->hide();
        this->ui->enableHighlights->show();
    });

    QObject::connect(this->ui->enableHighlights, &QPushButton::clicked, this, [=]() {
        QString str = app->settings->highlightUserBlacklist;
        QStringList list = str.split("\n");
        list.removeAll(this->ui->lblUsername->text());
        app->settings->highlightUserBlacklist = list.join("\n");
        this->ui->enableHighlights->hide();
        this->ui->disableHighlights->show();
    });

    this->updateButtons(this->ui->userLayout, false);
    this->updateButtons(this->ui->modLayout, false);
    this->updateButtons(this->ui->ownerLayout, false);

    // Close button
    QObject::connect(this->ui->btnClose, &QPushButton::clicked, [this] {
        this->hide();  //
    });

    this->scaleChangedEvent(this->getScale());
}

void AccountPopupWidget::setName(const QString &name)
{
    this->relationship.following = false;
    this->relationship.ignoring = false;

    this->popupWidgetUser.username = name;
    this->ui->lblUsername->setText(name);
    this->getUserId();

    // Refresh popup widget users type

    this->popupWidgetUser.refreshUserType(this->channel, false);
}

void AccountPopupWidget::User::refreshUserType(const ChannelPtr &channel, bool loggedInUser)
{
    if (channel->name == this->username) {
        this->userType = UserType::Owner;
    } else if ((loggedInUser && channel->isMod()) || channel->modList.contains(this->username)) {
        this->userType = UserType::Mod;
    } else {
        this->userType = UserType::User;
    }
}

void AccountPopupWidget::setChannel(ChannelPtr _channel)
{
    this->channel = _channel;
}

void AccountPopupWidget::getUserId()
{
    util::twitch::getUserID(this->popupWidgetUser.username, this, [=](const QString &id) {
        this->popupWidgetUser.userID = id;
        this->getUserData();
    });
}

void AccountPopupWidget::getUserData()
{
    util::twitch::get(
        "https://api.twitch.tv/kraken/channels/" + this->popupWidgetUser.userID, this,
        [=](const QJsonObject &obj) {
            this->ui->lblFollowers->setText(QString::number(obj.value("followers").toInt()));
            this->ui->lblViews->setText(QString::number(obj.value("views").toInt()));
            this->ui->lblAccountAge->setText(obj.value("created_at").toString().section("T", 0, 0));

            this->loadAvatar(QUrl(obj.value("logo").toString()));
        });

    util::twitch::get("https://api.twitch.tv/kraken/users/" + this->loggedInUser.userID +
                          "/follows/channels/" + this->popupWidgetUser.userID,
                      this, [=](const QJsonObject &obj) {
                          this->ui->follow->setEnabled(true);
                          this->relationship.following = obj.contains("channel");

                          emit refreshButtons();
                      });

    // TODO: Get ignore relationship between logged in user and popup widget user and update
    // relationship.ignoring
}

void AccountPopupWidget::loadAvatar(const QUrl &avatarUrl)
{
    if (!this->avatarMap.tryGet(this->popupWidgetUser.userID, this->avatar)) {
        if (!avatarUrl.isEmpty()) {
            QNetworkRequest req(avatarUrl);
            static auto manager = new QNetworkAccessManager();
            auto *reply = manager->get(req);

            QObject::connect(reply, &QNetworkReply::finished, this, [=] {
                if (reply->error() == QNetworkReply::NoError) {
                    const auto data = reply->readAll();
                    this->avatar.loadFromData(data);
                    this->avatarMap.insert(this->popupWidgetUser.userID, this->avatar);
                    this->ui->lblAvatar->setPixmap(this->avatar);
                } else {
                    this->ui->lblAvatar->setText("ERROR");
                }
            });
        } else {
            this->ui->lblAvatar->setText("No Avatar");
        }
    } else {
        this->ui->lblAvatar->setPixmap(this->avatar);
    }
}

void AccountPopupWidget::scaleChangedEvent(float newDpi)
{
    this->setStyleSheet(QString("* { font-size: <font-size>px; }")
                            .replace("<font-size>", QString::number((int)(12 * newDpi))));

    this->ui->lblAvatar->setFixedSize((int)(100 * newDpi), (int)(100 * newDpi));
}

void AccountPopupWidget::updateButtons(QWidget *layout, bool state)
{
    for (auto button : layout->findChildren<QPushButton *>()) {
        button->setVisible(state);
    }
}

void AccountPopupWidget::timeout(QPushButton *button, int time)
{
    QObject::connect(button, &QPushButton::clicked, this, [=]() {
        this->channel->sendMessage("/timeout " + this->ui->lblUsername->text() + " " +
                                   QString::number(time));
    });
}

void AccountPopupWidget::sendCommand(QPushButton *button, QString command)
{
    QObject::connect(button, &QPushButton::clicked, this, [=]() {
        this->channel->sendMessage(command + this->ui->lblUsername->text());
    });
}

void AccountPopupWidget::refreshLayouts()
{
    auto currentTwitchUser = getApp()->accounts->Twitch.getCurrent();
    if (!currentTwitchUser) {
        // No twitch user set (should never happen)
        return;
    }

    QString loggedInUsername = currentTwitchUser->getUserName();
    QString popupUsername = this->ui->lblUsername->text();

    bool showModLayout = false;
    bool showUserLayout = false;
    bool showOwnerLayout = false;

    if (loggedInUsername == popupUsername) {
        // Clicked user is the same as the logged in user
        showModLayout = false;
        showUserLayout = false;
        showOwnerLayout = false;
    } else {
        showUserLayout = true;

        switch (this->loggedInUser.userType) {
            case UserType::Mod: {
                showModLayout = true;
            } break;

            case UserType::Owner: {
                showModLayout = true;
                showOwnerLayout = true;
            } break;
        }
    }

    if (this->popupWidgetUser.userType == UserType::Owner) {
        showModLayout = false;
        showOwnerLayout = false;
    }

    if (this->popupWidgetUser.userType == UserType::Mod &&
        this->loggedInUser.userType != UserType::Owner) {
        showModLayout = false;
    }

    this->updateButtons(this->ui->modLayout, showModLayout);
    this->updateButtons(this->ui->userLayout, showUserLayout);
    this->updateButtons(this->ui->ownerLayout, showOwnerLayout);
}

void AccountPopupWidget::actuallyRefreshButtons()
{
    if (this->relationship.following) {
        if (this->ui->follow->text() != "Unfollow") {
            this->ui->follow->setText("Unfollow");
            this->ui->follow->setEnabled(true);
        }
    } else {
        if (this->ui->follow->text() != "Follow") {
            this->ui->follow->setText("Follow");
            this->ui->follow->setEnabled(true);
        }
    }

    if (this->relationship.ignoring) {
        if (this->ui->ignore->text() != "Unignore") {
            this->ui->ignore->setText("Unignore");
            this->ui->ignore->setEnabled(true);
        }
    } else {
        if (this->ui->ignore->text() != "Ignore") {
            this->ui->ignore->setText("Ignore");
            this->ui->ignore->setEnabled(true);
        }
    }
}

void AccountPopupWidget::focusOutEvent(QFocusEvent *)
{
    this->hide();
    this->ui->lblFollowers->setText("Loading...");
    this->ui->lblViews->setText("Loading...");
    this->ui->lblAccountAge->setText("Loading...");
    this->ui->lblUsername->setText("Loading...");
    this->ui->lblAvatar->setText("Loading...");
}

void AccountPopupWidget::showEvent(QShowEvent *)
{
    this->loggedInUser.refreshUserType(this->channel, true);
    this->popupWidgetUser.refreshUserType(this->channel, false);

    this->ui->follow->setEnabled(false);
    // XXX: Uncomment when ignore/unignore is fully implemented
    // this->ui->ignore->setEnabled(false);

    this->refreshButtons();

    this->refreshLayouts();

    QString blacklisted = getApp()->settings->highlightUserBlacklist;
    QStringList list = blacklisted.split("\n", QString::SkipEmptyParts);
    if (list.contains(this->ui->lblUsername->text(), Qt::CaseInsensitive)) {
        this->ui->disableHighlights->hide();
        this->ui->enableHighlights->show();
    } else {
        this->ui->disableHighlights->show();
        this->ui->enableHighlights->hide();
    }
}

}  // namespace widgets
}  // namespace chatterino
