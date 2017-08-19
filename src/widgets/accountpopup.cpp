#include "widgets/accountpopup.hpp"
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

AccountPopupWidget::AccountPopupWidget(std::shared_ptr<Channel> &channel)
    : QWidget(nullptr)
    , _ui(new Ui::AccountPopup)
    , _channel(channel)
{
    _ui->setupUi(this);

    resize(0, 0);

    setWindowFlags(Qt::FramelessWindowHint);

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

void AccountPopupWidget::focusOutEvent(QFocusEvent *event)
{
    hide();

    _ui->lblFollowers->setText("Loading...");
    _ui->lblViews->setText("Loading...");
    _ui->lblAccountAge->setText("Loading...");
    _ui->lblUsername->setText("Loading...");
    _ui->lblAvatar->setText("Loading...");
}

}  // namespace widgets
}  // namespace chatterino
