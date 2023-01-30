#pragma once

#include <magic_enum.hpp>
#include <QByteArray>
#include <QHash>
#include <QString>

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

struct Subscription {
    bool operator==(const Subscription &rhs) const;
    bool operator!=(const Subscription &rhs) const;
    QString condition;
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
struct hash<chatterino::seventv::eventapi::Subscription> {
    size_t operator()(
        const chatterino::seventv::eventapi::Subscription &sub) const
    {
        return (size_t)qHash(sub.condition, qHash((int)sub.type));
    }
};

}  // namespace std
