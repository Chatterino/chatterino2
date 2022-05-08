#pragma once

#include <QString>

#include <vector>

namespace chatterino {

// PubSubListenMessage is an outgoing LISTEN message that is sent for the client to subscribe to a list of topics
struct PubSubListenMessage {
    const std::vector<QString> topics;

    const QString nonce;

    QString token;

    PubSubListenMessage(std::vector<QString> _topics);

    void setToken(const QString &_token);

    QByteArray toJson() const;
};

}  // namespace chatterino
