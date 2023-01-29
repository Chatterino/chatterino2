#pragma once

#include <magic_enum.hpp>
#include <QByteArray>
#include <QHash>
#include <QString>

namespace chatterino {

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#subscription-types
enum class SeventvEventAPISubscriptionType {
    UpdateEmoteSet,
    UpdateUser,

    INVALID,
};

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#opcodes
enum class SeventvEventAPIOpcode {
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

struct SeventvEventAPISubscription {
    bool operator==(const SeventvEventAPISubscription &rhs) const;
    bool operator!=(const SeventvEventAPISubscription &rhs) const;
    QString condition;
    SeventvEventAPISubscriptionType type;

    QByteArray encodeSubscribe() const;
    QByteArray encodeUnsubscribe() const;

    friend QDebug &operator<<(QDebug &dbg,
                              const SeventvEventAPISubscription &subscription);
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::SeventvEventAPISubscriptionType>(
    chatterino::SeventvEventAPISubscriptionType value) noexcept
{
    switch (value)
    {
        case chatterino::SeventvEventAPISubscriptionType::UpdateEmoteSet:
            return "emote_set.update";
        case chatterino::SeventvEventAPISubscriptionType::UpdateUser:
            return "user.update";

        default:
            return default_tag;
    }
}

namespace std {

template <>
struct hash<chatterino::SeventvEventAPISubscription> {
    size_t operator()(const chatterino::SeventvEventAPISubscription &sub) const
    {
        return (size_t)qHash(sub.condition, qHash((int)sub.type));
    }
};

}  // namespace std
