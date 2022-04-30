#include "providers/twitch/pubsubmessages/Unlisten.hpp"

#include "util/Helpers.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace chatterino {

PubSubUnlistenMessage::PubSubUnlistenMessage(std::vector<QString> _topics)
    : topics(std::move(_topics))
    , nonce(generateUuid())
{
}

QByteArray PubSubUnlistenMessage::toJson() const
{
    QJsonObject root;

    root["type"] = "UNLISTEN";
    root["nonce"] = this->nonce;

    {
        QJsonObject data;

        QJsonArray jsonTopics;

        std::copy(this->topics.begin(), this->topics.end(),
                  std::back_inserter(jsonTopics));

        data["topics"] = jsonTopics;

        root["data"] = data;
    }

    return QJsonDocument(root).toJson();
}

}  // namespace chatterino
