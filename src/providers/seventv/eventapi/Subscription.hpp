#pragma once

#include <magic_enum.hpp>
#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QString>

#include <variant>

namespace chatterino::seventv::eventapi {

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#subscription-types
enum class SubscriptionType {
    UpdateEmoteSet,
    UpdateUser,

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

using Condition = std::variant<ObjectIDCondition>;

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
        case SubscriptionType::UpdateEmoteSet:
            return "emote_set.update";
        case SubscriptionType::UpdateUser:
            return "user.update";

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
