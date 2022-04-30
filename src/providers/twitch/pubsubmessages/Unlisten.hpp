#pragma once

#include <QString>

#include <vector>

namespace chatterino {

// PubSubUnlistenMessage is an outgoing UNLISTEN message that is sent for the client to unsubscribe from a list of topics
struct PubSubUnlistenMessage {
    const std::vector<QString> topics;

    const QString nonce;

    PubSubUnlistenMessage(std::vector<QString> _topics);

    QByteArray toJson() const;
};

}  // namespace chatterino
