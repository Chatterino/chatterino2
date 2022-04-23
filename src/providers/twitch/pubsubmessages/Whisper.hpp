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

    PubSubWhisperMessage(const QJsonObject &root)
        : typeString(root.value("type").toString())
    {
        auto oType =
            magic_enum::enum_cast<Type>(this->typeString.toStdString());
        if (oType.has_value())
        {
            this->type = oType.value();
        }

        // Parse information from data_object
        auto data = root.value("data_object").toObject();

        this->messageID = data.value("message_id").toString();
        this->id = data.value("id").toInt();
        this->threadID = data.value("thread_id").toString();
        this->body = data.value("body").toString();
        auto fromID = data.value("from_id");
        if (fromID.isString())
        {
            this->fromUserID = fromID.toString();
        }
        else
        {
            this->fromUserID = QString::number(data.value("from_id").toInt());
        }

        auto tags = data.value("tags").toObject();

        this->fromUserLogin = tags.value("login").toString();
        this->fromUserDisplayName = tags.value("display_name").toString();
        this->fromUserColor = QColor(tags.value("color").toString());
    }
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
