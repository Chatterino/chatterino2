#include "widgets/accountpopup.hpp"
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

AccountPopupWidget::AccountPopupWidget(SharedChannel _channel)
    : BaseWindow()
    , ui(new Ui::AccountPopup)
    , channel(_channel)
{
    this->ui->setupUi(this);

    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    this->setWindowFlags(Qt::FramelessWindowHint);

    this->resize(0, 0);

    singletons::SettingManager &settings = singletons::SettingManager::getInstance();

    this->permission = permissions::User;
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

    auto &accountManager = singletons::AccountManager::getInstance();
    QString userId;
    QString userNickname;
    auto currentTwitchUser = accountManager.Twitch.getCurrent();
    if (currentTwitchUser) {
        userId = currentTwitchUser->getUserId();
        userNickname = currentTwitchUser->getNickName();
    }

    QObject::connect(this->ui->profile, &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://twitch.tv/" + this->ui->lblUsername->text()));
    });

    QObject::connect(this->ui->sendMessage, &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(
            QUrl("https://www.twitch.tv/message/compose?to=" + this->ui->lblUsername->text()));
    });

    QObject::connect(this->ui->copy, &QPushButton::clicked, this,
                     [=]() { QApplication::clipboard()->setText(this->ui->lblUsername->text()); });

    QObject::connect(this->ui->follow, &QPushButton::clicked, this, [=]() {
        QUrl requestUrl("https://api.twitch.tv/kraken/users/" + userId + "/follows/channels/" +
                        this->userID);

        util::twitch::put(requestUrl,
                          [](QJsonObject obj) { qDebug() << "follows channel: " << obj; });
    });

    QObject::connect(this->ui->ignore, &QPushButton::clicked, this, [=]() {
        QUrl requestUrl("https://api.twitch.tv/kraken/users/" + userId + "/blocks/" + this->userID);

        util::twitch::put(requestUrl, [](QJsonObject obj) { qDebug() << "blocks user: " << obj; });
    });

    QObject::connect(this->ui->disableHighlights, &QPushButton::clicked, this, [=, &settings]() {
        QString str = settings.highlightUserBlacklist;
        str.append(this->ui->lblUsername->text() + "\n");
        settings.highlightUserBlacklist = str;
        this->ui->disableHighlights->hide();
        this->ui->enableHighlights->show();
    });

    QObject::connect(this->ui->enableHighlights, &QPushButton::clicked, this, [=, &settings]() {
        QString str = settings.highlightUserBlacklist;
        QStringList list = str.split("\n");
        list.removeAll(this->ui->lblUsername->text());
        settings.highlightUserBlacklist = list.join("\n");
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

    util::twitch::getUserID(userNickname, this,
                            [=](const QString &id) { currentTwitchUser->setUserId(id); });

    this->dpiMultiplierChanged(this->getDpiMultiplier(), this->getDpiMultiplier());
}

void AccountPopupWidget::setName(const QString &name)
{
    this->ui->lblUsername->setText(name);
    this->getUserId();
}

void AccountPopupWidget::setChannel(SharedChannel _channel)
{
    this->channel = _channel;
}

void AccountPopupWidget::getUserId()
{
    util::twitch::getUserID(this->ui->lblUsername->text(), this, [=](const QString &id) {
        userID = id;
        this->getUserData();
    });
}

void AccountPopupWidget::getUserData()
{
    util::twitch::get(
        "https://api.twitch.tv/kraken/channels/" + this->userID, this, [=](const QJsonObject &obj) {
            this->ui->lblFollowers->setText(QString::number(obj.value("followers").toInt()));
            this->ui->lblViews->setText(QString::number(obj.value("views").toInt()));
            this->ui->lblAccountAge->setText(obj.value("created_at").toString().section("T", 0, 0));

            this->loadAvatar(QUrl(obj.value("logo").toString()));
        });
}

void AccountPopupWidget::loadAvatar(const QUrl &avatarUrl)
{
    if (!this->avatarMap.tryGet(this->userID, this->avatar)) {
        if (!avatarUrl.isEmpty()) {
            QNetworkRequest req(avatarUrl);
            static auto manager = new QNetworkAccessManager();
            auto *reply = manager->get(req);

            QObject::connect(reply, &QNetworkReply::finished, this, [=] {
                if (reply->error() == QNetworkReply::NoError) {
                    const auto data = reply->readAll();
                    this->avatar.loadFromData(data);
                    this->avatarMap.insert(this->userID, this->avatar);
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

void AccountPopupWidget::updatePermissions()
{
    singletons::AccountManager &accountManager = singletons::AccountManager::getInstance();

    auto currentTwitchUser = accountManager.Twitch.getCurrent();
    if (!currentTwitchUser) {
        // No twitch user set (should never happen)
        return;
    }

    if (this->channel.get()->name == currentTwitchUser->getNickName()) {
        this->permission = permissions::Owner;
    } else if (this->channel->modList.contains(currentTwitchUser->getNickName())) {
        // XXX(pajlada): This might always trigger if user is anonymous (if nickName is empty?)
        this->permission = permissions::Mod;
    }
}

void AccountPopupWidget::dpiMultiplierChanged(float /*oldDpi*/, float newDpi)
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
    singletons::AccountManager &accountManager = singletons::AccountManager::getInstance();
    auto currentTwitchUser = accountManager.Twitch.getCurrent();
    if (!currentTwitchUser) {
        // No twitch user set (should never happen)
        return;
    }

    if (this->ui->lblUsername->text() != currentTwitchUser->getNickName()) {
        this->updateButtons(this->ui->userLayout, true);
        if (this->permission != permissions::User) {
            if (!this->channel->modList.contains(this->ui->lblUsername->text())) {
                this->updateButtons(this->ui->modLayout, true);
            }
            if (this->permission == permissions::Owner) {
                this->updateButtons(this->ui->ownerLayout, true);
                this->updateButtons(this->ui->modLayout, true);
            }
        }
    } else {
        this->updateButtons(this->ui->modLayout, false);
        this->updateButtons(this->ui->userLayout, false);
        this->updateButtons(this->ui->ownerLayout, false);
    }

    QString blacklisted = singletons::SettingManager::getInstance().highlightUserBlacklist;
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
