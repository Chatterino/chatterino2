#pragma once

#include <magic_enum/magic_enum.hpp>
#include <QColor>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <set>

namespace chatterino {

struct PubSubAutoModQueueMessage {
    enum class Type {
        AutoModCaughtMessage,

        INVALID,
    };

    enum class Reason {
        AutoMod,
        BlockedTerm,

        INVALID,
    };

    QString typeString;
    Type type = Type::INVALID;
    Reason reason = Reason::INVALID;

    QJsonObject data;

    QString status;

    QString contentCategory;
    int contentLevel{};

    QString messageID;
    QString messageText;

    QString senderUserID;
    QString senderUserLogin;
    QString senderUserDisplayName;
    QColor senderUserChatColor;

    std::set<QString> blockedTermsFound;

    PubSubAutoModQueueMessage() = default;
    explicit PubSubAutoModQueueMessage(const QJsonObject &root);
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

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubAutoModQueueMessage::Reason>(
    chatterino::PubSubAutoModQueueMessage::Reason value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubAutoModQueueMessage::Reason::AutoMod:
            return "AutoModCaughtMessageReason";
        case chatterino::PubSubAutoModQueueMessage::Reason::BlockedTerm:
            return "BlockedTermCaughtMessageReason";

        default:
            return default_tag;
    }
}
