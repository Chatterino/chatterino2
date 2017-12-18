#include "widgets/accountpopup.hpp"
#include "accountmanager.hpp"
#include "channel.hpp"
#include "credentials.hpp"
#include "settingsmanager.hpp"
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

AccountPopupWidget::AccountPopupWidget(std::shared_ptr<Channel> channel)
    : BaseWidget()
    , _ui(new Ui::AccountPopup)
    , _channel(channel)
{
    _ui->setupUi(this);

    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    setWindowFlags(Qt::FramelessWindowHint);
    this->initAsWindow();

    resize(0, 0);

    SettingsManager &settings = SettingsManager::getInstance();

    permission = permissions::User;
    for (auto button : this->_ui->profileLayout->findChildren<QPushButton *>()) {
        button->setFocusProxy(this);
    }
    for (auto button : this->_ui->userLayout->findChildren<QPushButton *>()) {
        button->setFocusProxy(this);
    }
    for (auto button : this->_ui->modLayout->findChildren<QPushButton *>()) {
        button->setFocusProxy(this);
    }
    for (auto button : this->_ui->ownerLayout->findChildren<QPushButton *>()) {
        button->setFocusProxy(this);
    }

    timeout(this->_ui->purge, 1);
    timeout(this->_ui->min1, 60);
    timeout(this->_ui->min10, 600);
    timeout(this->_ui->hour1, 3600);
    timeout(this->_ui->hour24, 86400);

    sendCommand(this->_ui->ban, "/ban ");
    sendCommand(this->_ui->unBan, "/unban ");
    sendCommand(this->_ui->mod, "/mod ");
    sendCommand(this->_ui->unMod, "/unmod ");

    auto &accountManager = AccountManager::getInstance();
    QString userId;
    QString userNickname;
    auto currentTwitchUser = accountManager.Twitch.getCurrent();
    if (currentTwitchUser) {
        userId = currentTwitchUser->getUserId();
        userNickname = currentTwitchUser->getNickName();
    }

    QObject::connect(this->_ui->profile, &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://twitch.tv/" + this->_ui->lblUsername->text()));
    });

    QObject::connect(this->_ui->sendMessage, &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(
            QUrl("https://www.twitch.tv/message/compose?to=" + this->_ui->lblUsername->text()));
    });

    QObject::connect(this->_ui->copy, &QPushButton::clicked, this,
                     [=]() { QApplication::clipboard()->setText(this->_ui->lblUsername->text()); });

    QObject::connect(this->_ui->follow, &QPushButton::clicked, this, [=]() {
        QUrl requestUrl("https://api.twitch.tv/kraken/users/" + userId + "/follows/channels/" +
                        this->userID);

        util::twitch::put(requestUrl,
                          [](QJsonObject obj) { qDebug() << "follows channel: " << obj; });
    });

    QObject::connect(this->_ui->ignore, &QPushButton::clicked, this, [=]() {
        QUrl requestUrl("https://api.twitch.tv/kraken/users/" + userId + "/blocks/" + this->userID);

        util::twitch::put(requestUrl, [](QJsonObject obj) { qDebug() << "blocks user: " << obj; });
    });

    QObject::connect(this->_ui->disableHighlights, &QPushButton::clicked, this, [=, &settings]() {
        QString str = settings.highlightUserBlacklist.getnonConst();
        str.append(this->_ui->lblUsername->text() + "\n");
        settings.highlightUserBlacklist.set(str);
        this->_ui->disableHighlights->hide();
        this->_ui->enableHighlights->show();
    });

    QObject::connect(this->_ui->enableHighlights, &QPushButton::clicked, this, [=, &settings]() {
        QString str = settings.highlightUserBlacklist.getnonConst();
        QStringList list = str.split("\n");
        list.removeAll(this->_ui->lblUsername->text());
        settings.highlightUserBlacklist.set(list.join("\n"));
        this->_ui->enableHighlights->hide();
        this->_ui->disableHighlights->show();
    });

    updateButtons(this->_ui->userLayout, false);
    updateButtons(this->_ui->modLayout, false);
    updateButtons(this->_ui->ownerLayout, false);

    // Close button
    connect(_ui->btnClose, &QPushButton::clicked, [=]() {
        hide();  //
    });

    util::twitch::getUserID(userNickname, this,
                            [=](const QString &id) { currentTwitchUser->setUserId(id); });

    this->dpiMultiplyerChanged(this->getDpiMultiplier(), this->getDpiMultiplier());
}

void AccountPopupWidget::setName(const QString &name)
{
    _ui->lblUsername->setText(name);
    getUserId();
}

void AccountPopupWidget::setChannel(std::shared_ptr<Channel> channel)
{
    this->_channel = channel;
}

void AccountPopupWidget::getUserId()
{
    util::twitch::getUserID(this->_ui->lblUsername->text(), this, [=](const QString &id) {
        userID = id;
        getUserData();
    });
}

void AccountPopupWidget::getUserData()
{
    util::twitch::get(
        "https://api.twitch.tv/kraken/channels/" + userID, this, [=](const QJsonObject &obj) {
            _ui->lblFollowers->setText(QString::number(obj.value("followers").toInt()));
            _ui->lblViews->setText(QString::number(obj.value("views").toInt()));
            _ui->lblAccountAge->setText(obj.value("created_at").toString().section("T", 0, 0));

            loadAvatar(QUrl(obj.value("logo").toString()));
        });
}

void AccountPopupWidget::loadAvatar(const QUrl &avatarUrl)
{
    if (!avatarMap.tryGet(userID, this->avatar)) {
        if (!avatarUrl.isEmpty()) {
            QNetworkRequest req(avatarUrl);
            static auto manager = new QNetworkAccessManager();
            auto *reply = manager->get(req);

            QObject::connect(reply, &QNetworkReply::finished, this, [=] {
                if (reply->error() == QNetworkReply::NoError) {
                    const auto data = reply->readAll();
                    this->avatar.loadFromData(data);
                    this->avatarMap.insert(userID, avatar);
                    _ui->lblAvatar->setPixmap(avatar);
                } else {
                    _ui->lblAvatar->setText("ERROR");
                }
            });
        } else {
            _ui->lblAvatar->setText("No Avatar");
        }
    } else {
        _ui->lblAvatar->setPixmap(this->avatar);
    }
}

void AccountPopupWidget::updatePermissions()
{
    AccountManager &accountManager = AccountManager::getInstance();
    auto currentTwitchUser = accountManager.Twitch.getCurrent();
    if (!currentTwitchUser) {
        // No twitch user set (should never happen)
        return;
    }

    if (this->_channel.get()->name == currentTwitchUser->getNickName()) {
        permission = permissions::Owner;
    } else if (this->_channel->modList.contains(currentTwitchUser->getNickName())) {
        // XXX(pajlada): This might always trigger if user is anonymous (if nickName is empty?)
        permission = permissions::Mod;
    }
}

void AccountPopupWidget::dpiMultiplyerChanged(float oldDpi, float newDpi)
{
    this->setStyleSheet(QString("* { font-size: <font-size>px; }")
                            .replace("<font-size>", QString::number((int)(12 * newDpi))));

    this->_ui->lblAvatar->setFixedSize((int)(100 * newDpi), (int)(100 * newDpi));
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
        this->_channel->sendMessage("/timeout " + this->_ui->lblUsername->text() + " " +
                                    QString::number(time));
    });
}

void AccountPopupWidget::sendCommand(QPushButton *button, QString command)
{
    QObject::connect(button, &QPushButton::clicked, this, [=]() {
        this->_channel->sendMessage(command + this->_ui->lblUsername->text());
    });
}

void AccountPopupWidget::focusOutEvent(QFocusEvent *event)
{
    this->hide();
    _ui->lblFollowers->setText("Loading...");
    _ui->lblViews->setText("Loading...");
    _ui->lblAccountAge->setText("Loading...");
    _ui->lblUsername->setText("Loading...");
    _ui->lblAvatar->setText("Loading...");
}

void AccountPopupWidget::showEvent(QShowEvent *event)
{
    AccountManager &accountManager = AccountManager::getInstance();
    auto currentTwitchUser = accountManager.Twitch.getCurrent();
    if (!currentTwitchUser) {
        // No twitch user set (should never happen)
        return;
    }

    if (this->_ui->lblUsername->text() != currentTwitchUser->getNickName()) {
        updateButtons(this->_ui->userLayout, true);
        if (permission != permissions::User) {
            if (!this->_channel->modList.contains(this->_ui->lblUsername->text())) {
                updateButtons(this->_ui->modLayout, true);
            }
            if (permission == permissions::Owner) {
                updateButtons(this->_ui->ownerLayout, true);
                updateButtons(this->_ui->modLayout, true);
            }
        }
    } else {
        updateButtons(this->_ui->modLayout, false);
        updateButtons(this->_ui->userLayout, false);
        updateButtons(this->_ui->ownerLayout, false);
    }

    QString blacklisted = SettingsManager::getInstance().highlightUserBlacklist.getnonConst();
    QStringList list = blacklisted.split("\n", QString::SkipEmptyParts);
    if (list.contains(this->_ui->lblUsername->text(), Qt::CaseInsensitive)) {
        this->_ui->disableHighlights->hide();
        this->_ui->enableHighlights->show();
    } else {
        this->_ui->disableHighlights->show();
        this->_ui->enableHighlights->hide();
    }
}

}  // namespace widgets
}  // namespace chatterino
