#pragma once

#include "common/QLogging.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <optional>

namespace chatterino {

struct PubSubMessageMessage {
    QString nonce;
    QString topic;

    QJsonObject messageObject;

    PubSubMessageMessage(QString _nonce, const QJsonObject &data)
        : nonce(std::move(_nonce))
        , topic(data.value("topic").toString())
    {
        auto messagePayload = data.value("message").toString().toUtf8();

        auto messageDoc = QJsonDocument::fromJson(messagePayload);

        if (messageDoc.isNull())
        {
            qCWarning(chatterinoPubSub) << "PubSub message (type MESSAGE) "
                                           "missing inner message payload";
            return;
        }

        if (!messageDoc.isObject())
        {
            qCWarning(chatterinoPubSub)
                << "PubSub message (type MESSAGE) inner message payload is not "
                   "an object";
            return;
        }

        this->messageObject = messageDoc.object();
    }

    template <class InnerClass>
    std::optional<InnerClass> toInner() const;
};

template <class InnerClass>
std::optional<InnerClass> PubSubMessageMessage::toInner() const
{
    if (this->messageObject.empty())
    {
        return std::nullopt;
    }

    return InnerClass{this->messageObject};
}

}  // namespace chatterino
