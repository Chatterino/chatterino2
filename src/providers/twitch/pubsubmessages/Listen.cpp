#include "providers/twitch/pubsubmessages/Listen.hpp"

#include "util/Helpers.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace chatterino {

PubSubListenMessage::PubSubListenMessage(std::vector<QString> _topics)
    : topics(std::move(_topics))
    , nonce(generateUuid())
{
}

void PubSubListenMessage::setToken(const QString &_token)
{
    this->token = _token;
}

QByteArray PubSubListenMessage::toJson() const
{
    QJsonObject root;

    root["type"] = "LISTEN";
    root["nonce"] = this->nonce;

    {
        QJsonObject data;

        QJsonArray jsonTopics;

        std::copy(this->topics.begin(), this->topics.end(),
                  std::back_inserter(jsonTopics));

        data["topics"] = jsonTopics;

        if (!this->token.isEmpty())
        {
            data["auth_token"] = this->token;
        }

        root["data"] = data;
    }

    return QJsonDocument(root).toJson();
}

}  // namespace chatterino
