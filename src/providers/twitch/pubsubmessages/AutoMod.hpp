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

    PubSubAutoModQueueMessage(const QJsonObject &root);
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
