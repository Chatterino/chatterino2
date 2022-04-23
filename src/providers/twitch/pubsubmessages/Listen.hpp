#pragma once

#include "util/Helpers.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <vector>

namespace chatterino {

// PubSubListenMessage is an outgoing LISTEN message that is sent for the client to subscribe to a list of topics
struct PubSubListenMessage {
    const std::vector<QString> topics;

    const QString nonce;

    QString token;

    PubSubListenMessage(std::vector<QString> _topics)
        : topics(std::move(_topics))
        , nonce(generateUuid())
    {
    }

    void setToken(const QString &_token)
    {
        this->token = _token;
    }

    QByteArray toJson() const
    {
        QJsonObject root;

        root.value("type") = "LISTEN";

        root.value("nonce") = this->nonce;

        if (!this->token.isEmpty())
        {
            root.value("auth_token") = this->token;
        }

        {
            QJsonObject data;

            QJsonArray jsonTopics;

            std::copy(this->topics.begin(), this->topics.end(),
                      std::back_inserter(jsonTopics));

            data.value("topics") = jsonTopics;

            root.value("data") = data;
        }

        QJsonDocument doc(root);

        return doc.toJson();
    }
};

}  // namespace chatterino
