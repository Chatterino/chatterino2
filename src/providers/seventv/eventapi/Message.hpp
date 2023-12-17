#pragma once

#include "providers/seventv/eventapi/Subscription.hpp"

#include <magic_enum/magic_enum.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <optional>

namespace chatterino::seventv::eventapi {

struct Message {
    QJsonObject data;

    Opcode op;

    Message(QJsonObject _json);

    template <class InnerClass>
    std::optional<InnerClass> toInner();
};

template <class InnerClass>
std::optional<InnerClass> Message::toInner()
{
    return InnerClass{this->data};
}

static std::optional<Message> parseBaseMessage(const QString &blob)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(blob.toUtf8()));

    if (jsonDoc.isNull())
    {
        return std::nullopt;
    }

    return Message(jsonDoc.object());
}

}  // namespace chatterino::seventv::eventapi
