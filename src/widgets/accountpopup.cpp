#include "widgets/accountpopup.hpp"
#include "accountmanager.hpp"
#include "channel.hpp"
#include "credentials.hpp"
#include "ui_accountpopupform.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>

namespace chatterino {
namespace widgets {

AccountPopupWidget::AccountPopupWidget(std::shared_ptr<Channel> channel)
    : QWidget(nullptr)
    , _ui(new Ui::AccountPopup)
    , _channel(channel)
{
    _ui->setupUi(this);

    resize(0, 0);

    setWindowFlags(Qt::FramelessWindowHint);

    permission = permissions::User;
    for(auto button : this->_ui->profileLayout->findChildren<QPushButton*>())
    {
        button->setFocusProxy(this);
    }
    for(auto button: this->_ui->userLayout->findChildren<QPushButton*>())
    {
        button->setFocusProxy(this);
    }
    for(auto button: this->_ui->modLayout->findChildren<QPushButton*>())
    {
        button->setFocusProxy(this);
    }
    for(auto button: this->_ui->ownerLayout->findChildren<QPushButton*>())
    {
        button->setFocusProxy(this);
    }

    timeout(this->_ui->purge,1);
    timeout(this->_ui->min1,60);
    timeout(this->_ui->min10,600);
    timeout(this->_ui->hour1,3600);
    timeout(this->_ui->hour24,86400);

    sendCommand(this->_ui->ban,"/ban ");
    sendCommand(this->_ui->unBan,"/unban ");

    sendCommand(this->_ui->mod,"/mod ");
    sendCommand(this->_ui->unMod,"/unmod ");


    updateButtons(this->_ui->userLayout,false);
    updateButtons(this->_ui->modLayout,false);
    updateButtons(this->_ui->ownerLayout,false);

    // Close button
    connect(_ui->btnClose, &QPushButton::clicked, [=]() {
        hide();  //
    });
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
    QUrl nameUrl("https://api.twitch.tv/kraken/users?login=" + _ui->lblUsername->text());

    QNetworkRequest req(nameUrl);
    req.setRawHeader(QByteArray("Accept"), QByteArray("application/vnd.twitchtv.v5+json"));
    req.setRawHeader(QByteArray("Client-ID"), getDefaultClientID());

    static auto manager = new QNetworkAccessManager();
    auto *reply = manager->get(req);

    QObject::connect(reply, &QNetworkReply::finished, this, [=] {
        if (reply->error() == QNetworkReply::NoError) {
            auto doc = QJsonDocument::fromJson(reply->readAll());
            auto obj = doc.object();
            auto array = obj.value("users").toArray();

            userID = array.at(0).toObject().value("_id").toString();

            getUserData();
        }
    });
}

void AccountPopupWidget::getUserData()
{
    QUrl idUrl("https://api.twitch.tv/kraken/channels/" + userID);

    QNetworkRequest req(idUrl);
    req.setRawHeader(QByteArray("Accept"), QByteArray("application/vnd.twitchtv.v5+json"));
    req.setRawHeader(QByteArray("Client-ID"), getDefaultClientID());

    static auto manager = new QNetworkAccessManager();
    auto *reply = manager->get(req);

    QObject::connect(reply, &QNetworkReply::finished, this, [=] {
        if (reply->error() == QNetworkReply::NoError) {
            auto doc = QJsonDocument::fromJson(reply->readAll());
            auto obj = doc.object();

            _ui->lblFollowers->setText(QString::number(obj.value("followers").toInt()));
            _ui->lblViews->setText(QString::number(obj.value("views").toInt()));
            _ui->lblAccountAge->setText(obj.value("created_at").toString().section("T", 0, 0));

            loadAvatar(QUrl(obj.value("logo").toString()));
        } else {
            _ui->lblFollowers->setText("ERROR");
            _ui->lblViews->setText("ERROR");
            _ui->lblAccountAge->setText("ERROR");
        }
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
    if(this->_channel.get()->name == AccountManager::getInstance().getTwitchUser().getNickName())
    {
        permission = permissions::Owner;
    }
    else if(this->_channel->modList.contains(AccountManager::getInstance().getTwitchUser().getNickName()))
    {
        permission = permissions::Mod;
    }
}

void AccountPopupWidget::updateButtons(QWidget* layout, bool state)
{
    for(auto button : layout->findChildren<QPushButton*>())
    {
        button->setVisible(state);
    }
}

void AccountPopupWidget::timeout(QPushButton *button, int time)
{
    QObject::connect(button, &QPushButton::clicked, this, [=](){
       this->_channel->sendMessage("/timeout " + this->_ui->lblUsername->text() + " " + QString::number(time));
    });
}

void AccountPopupWidget::sendCommand(QPushButton *button, QString command)
{
    QObject::connect(button, &QPushButton::clicked, this, [=](){
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
    if(this->_ui->lblUsername->text() != AccountManager::getInstance().getTwitchUser().getNickName())
    {    
        updateButtons(this->_ui->userLayout, true);
        if(permission != permissions::User)
        {
            if(!this->_channel->modList.contains(this->_ui->lblUsername->text()))
            {
                updateButtons(this->_ui->modLayout, true);
            }
            if(permission == permissions::Owner)
            {
                updateButtons(this->_ui->ownerLayout, true);
                updateButtons(this->_ui->modLayout, true);
            }
        }
    }
    else
    {
        updateButtons(this->_ui->modLayout, false);
        updateButtons(this->_ui->userLayout, false);
        updateButtons(this->_ui->ownerLayout, false);
    }
}

}  // namespace widgets
}  // namespace chatterino
