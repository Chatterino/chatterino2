#pragma once

#include <QColor>
#include <QJsonObject>
#include <QString>

#include <magic_enum.hpp>

namespace chatterino {

struct PubSubAutoModQueueMessage {
    enum class Type {
        AutoModCaughtMessage,

        INVALID,
    };
    QString typeString;
    Type type = Type::INVALID;

    QJsonObject data;

    QString status;

    QString contentCategory;
    int contentLevel;

    QString messageID;
    QString messageText;

    QString senderUserID;
    QString senderUserLogin;
    QString senderUserDisplayName;
    QColor senderUserChatColor;

    PubSubAutoModQueueMessage(const QJsonObject &root)
        : typeString(root.value("type").toString())
        , data(root.value("data").toObject())
        , status(this->data.value("status").toString())
    {
        auto oType =
            magic_enum::enum_cast<Type>(this->typeString.toStdString());
        if (oType.has_value())
        {
            this->type = oType.value();
        }

        auto contentClassification =
            data.value("content_classification").toObject();

        this->contentCategory =
            contentClassification.value("category").toString();
        this->contentLevel = contentClassification.value("level").toInt();

        auto message = data.value("message").toObject();

        this->messageID = message.value("id").toString();

        auto messageContent = message.value("content").toObject();

        this->messageText = messageContent.value("text").toString();

        auto messageSender = message.value("sender").toObject();

        this->senderUserID = messageSender.value("user_id").toString();
        this->senderUserLogin = messageSender.value("login").toString();
        this->senderUserDisplayName =
            messageSender.value("display_name").toString();
        this->senderUserChatColor =
            QColor(messageSender.value("chat_color").toString());
    }
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubAutoModQueueMessage::Type>(
    chatterino::PubSubAutoModQueueMessage::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubAutoModQueueMessage::Type::AutoModCaughtMessage:
            return "automod_caught_message";

        default:
            return default_tag;
    }
}
