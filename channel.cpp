#include "channel.h"
#include "messages/message.h"
#include "windows.h"

#include <memory>

namespace chatterino {

Channel::Channel(const QString &channel)
    : messages()
    , name((channel.length() > 0 && channel[0] == '#') ? channel.mid(1)
                                                       : channel)
    , bttvChannelEmotes()
    , ffzChannelEmotes()
    , messageMutex()
    , subLink("https://www.twitch.tv/" + name +
              "/subscribe?ref=in_chat_subscriber_link")
    , channelLink("https://twitch.tv/" + name)
    , popoutPlayerLink("https://player.twitch.tv/?channel=" + name)
{
    //    for (int i = 0; i < 40; i++) {
    //        addMessage(std::shared_ptr<messages::Message>(
    //            new messages::Message("test xD test")));
    //    }
}

void
Channel::reloadBttvEmotes()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QUrl url("https://api.frankerfacez.com/v1/set/global");
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
            QJsonObject root = jsonDoc.object();

            auto sets = root.value("sets").toObject();

            for (const QJsonValue &set : sets) {
                auto emoticons = set.toObject().value("emoticons").toArray();

                for (const QJsonValue &emote : emoticons) {
                    QJsonObject object = emote.toObject();

                    // margins

                    int id = object.value("id").toInt();
                    QString code = object.value("name").toString();

                    QJsonObject urls = object.value("urls").toObject();
                    QString url1 = "http:" + urls.value("1").toString();

                    Emotes::getBttvEmotes().insert(
                        code, new messages::LazyLoadedImage(
                                  url1, 1, code, code + "\nGlobal Ffz Emote"));
                }
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

void
Channel::reloadFfzEmotes()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QUrl url("https://api.frankerfacez.com/v1/set/global");
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
            QJsonObject root = jsonDoc.object();

            auto sets = root.value("sets").toObject();

            for (const QJsonValue &set : sets) {
                auto emoticons = set.toObject().value("emoticons").toArray();

                for (const QJsonValue &emote : emoticons) {
                    QJsonObject object = emote.toObject();

                    // margins

                    int id = object.value("id").toInt();
                    QString code = object.value("name").toString();

                    QJsonObject urls = object.value("urls").toObject();
                    QString url1 = "http:" + urls.value("1").toString();

                    Emotes::getBttvEmotes().insert(
                        code, new messages::LazyLoadedImage(
                                  url1, 1, code, code + "\nGlobal Ffz Emote"));
                }
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

QVector<std::shared_ptr<messages::Message>>
Channel::getMessagesClone()
{
    this->messageMutex.lock();
    QVector<std::shared_ptr<messages::Message>> M(this->messages);
    M.detach();
    this->messageMutex.unlock();
    return M;
}

void
Channel::addMessage(std::shared_ptr<messages::Message> message)
{
    this->messageMutex.lock();
    this->messages.append(message);
    this->messageMutex.unlock();

    Windows::repaintVisibleChatWidgets();
}
}
