#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <magic_enum.hpp>

#include <boost/optional.hpp>

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
    boost::optional<InnerClass> toInner();
};

template <class InnerClass>
boost::optional<InnerClass> PubSubMessage::toInner()
{
    auto dataValue = this->object.value("data");
    if (!dataValue.isObject())
    {
        return boost::none;
    }

    auto data = dataValue.toObject();

    return InnerClass{this->nonce, data};
}

static boost::optional<PubSubMessage> parsePubSubBaseMessage(
    const QString &blob)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(blob.toUtf8()));

    if (jsonDoc.isNull())
    {
        return boost::none;
    }

    return PubSubMessage(jsonDoc.object());
}

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
