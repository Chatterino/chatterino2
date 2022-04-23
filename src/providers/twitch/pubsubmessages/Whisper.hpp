#pragma once

#include <QColor>
#include <QJsonObject>
#include <QString>

#include <magic_enum.hpp>

namespace chatterino {

struct PubSubWhisperMessage {
    enum class Type {
        WhisperReceived,
        WhisperSent,
        Thread,

        INVALID,
    };

    QString typeString;
    Type type = Type::INVALID;

    QString messageID;
    int id;
    QString threadID;
    QString body;
    QString fromUserID;
    QString fromUserLogin;
    QString fromUserDisplayName;
    QColor fromUserColor;

    PubSubWhisperMessage(const QJsonObject &root);
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<chatterino::PubSubWhisperMessage::Type>(
        chatterino::PubSubWhisperMessage::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubWhisperMessage::Type::WhisperReceived:
            return "whisper_received";

        case chatterino::PubSubWhisperMessage::Type::WhisperSent:
            return "whisper_sent";

        case chatterino::PubSubWhisperMessage::Type::Thread:
            return "thread";
        default:
            return default_tag;
    }
}
