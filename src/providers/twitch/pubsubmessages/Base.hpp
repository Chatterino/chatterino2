#pragma once

#include <magic_enum/magic_enum.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <optional>

namespace chatterino {

struct PubSubMessage {
    enum class Type {
        Pong,
        Response,
        Message,

        INVALID,
    };

    QJsonObject object;

    QString nonce;
    QString error;
    QString typeString;
    Type type;

    PubSubMessage(QJsonObject _object);

    template <class InnerClass>
    std::optional<InnerClass> toInner();
};

template <class InnerClass>
std::optional<InnerClass> PubSubMessage::toInner()
{
    auto dataValue = this->object.value("data");
    if (!dataValue.isObject())
    {
        return std::nullopt;
    }

    auto data = dataValue.toObject();

    return InnerClass{this->nonce, data};
}

std::optional<PubSubMessage> parsePubSubBaseMessage(const QString &blob);

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<chatterino::PubSubMessage::Type>(
        chatterino::PubSubMessage::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubMessage::Type::Pong:
            return "PONG";

        case chatterino::PubSubMessage::Type::Response:
            return "RESPONSE";

        case chatterino::PubSubMessage::Type::Message:
            return "MESSAGE";

        default:
            return default_tag;
    }
}
