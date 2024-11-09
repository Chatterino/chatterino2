#pragma once

#include <magic_enum/magic_enum.hpp>
#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QString>

#include <variant>

namespace chatterino::seventv::eventapi {

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#subscription-types
enum class SubscriptionType {
    AnyEmoteSet,
    CreateEmoteSet,
    UpdateEmoteSet,

    UpdateUser,

    AnyCosmetic,
    CreateCosmetic,
    UpdateCosmetic,
    DeleteCosmetic,

    AnyEntitlement,
    CreateEntitlement,
    UpdateEntitlement,
    DeleteEntitlement,
    ResetEntitlement,

    INVALID,
};

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#opcodes
enum class Opcode {
    Dispatch = 0,
    Hello = 1,
    Heartbeat = 2,
    Reconnect = 4,
    Ack = 5,
    Error = 6,
    EndOfStream = 7,
    Identify = 33,
    Resume = 34,
    Subscribe = 35,
    Unsubscribe = 36,
    Signal = 37,
};

struct ObjectIDCondition {
    ObjectIDCondition(QString objectID);

    QString objectID;

    QJsonObject encode() const;

    friend QDebug &operator<<(QDebug &dbg, const ObjectIDCondition &condition);
    bool operator==(const ObjectIDCondition &rhs) const;
    bool operator!=(const ObjectIDCondition &rhs) const;
};

struct ChannelCondition {
    ChannelCondition(QString twitchID);

    QString twitchID;

    QJsonObject encode() const;

    friend QDebug &operator<<(QDebug &dbg, const ChannelCondition &condition);
    bool operator==(const ChannelCondition &rhs) const;
    bool operator!=(const ChannelCondition &rhs) const;
};

using Condition = std::variant<ObjectIDCondition, ChannelCondition>;

struct Subscription {
    bool operator==(const Subscription &rhs) const;
    bool operator!=(const Subscription &rhs) const;
    Condition condition;
    SubscriptionType type;

    QByteArray encodeSubscribe() const;
    QByteArray encodeUnsubscribe() const;

    friend QDebug &operator<<(QDebug &dbg, const Subscription &subscription);
};

}  // namespace chatterino::seventv::eventapi

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::seventv::eventapi::SubscriptionType>(
    chatterino::seventv::eventapi::SubscriptionType value) noexcept
{
    using chatterino::seventv::eventapi::SubscriptionType;
    switch (value)
    {
        case SubscriptionType::AnyEmoteSet:
            return "emote_set.*";
        case SubscriptionType::CreateEmoteSet:
            return "emote_set.create";
        case SubscriptionType::UpdateEmoteSet:
            return "emote_set.update";
        case SubscriptionType::UpdateUser:
            return "user.update";
        case SubscriptionType::AnyCosmetic:
            return "cosmetic.*";
        case SubscriptionType::CreateCosmetic:
            return "cosmetic.create";
        case SubscriptionType::UpdateCosmetic:
            return "cosmetic.update";
        case SubscriptionType::DeleteCosmetic:
            return "cosmetic.delete";
        case SubscriptionType::AnyEntitlement:
            return "entitlement.*";
        case SubscriptionType::CreateEntitlement:
            return "entitlement.create";
        case SubscriptionType::UpdateEntitlement:
            return "entitlement.update";
        case SubscriptionType::DeleteEntitlement:
            return "entitlement.delete";
        case SubscriptionType::ResetEntitlement:
            return "entitlement.reset";

        default:
            return default_tag;
    }
}

namespace std {

template <>
struct hash<chatterino::seventv::eventapi::ObjectIDCondition> {
    size_t operator()(
        const chatterino::seventv::eventapi::ObjectIDCondition &c) const
    {
        return (size_t)qHash(c.objectID);
    }
};

template <>
struct hash<chatterino::seventv::eventapi::ChannelCondition> {
    size_t operator()(
        const chatterino::seventv::eventapi::ChannelCondition &c) const
    {
        return qHash(c.twitchID);
    }
};

template <>
struct hash<chatterino::seventv::eventapi::Subscription> {
    size_t operator()(
        const chatterino::seventv::eventapi::Subscription &sub) const
    {
        const size_t conditionHash =
            std::hash<chatterino::seventv::eventapi::Condition>{}(
                sub.condition);
        return (size_t)qHash(conditionHash, qHash((int)sub.type));
    }
};

}  // namespace std
