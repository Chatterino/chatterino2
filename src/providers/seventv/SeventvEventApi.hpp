#pragma once

#include <QHash>
#include <QString>
#include "magic_enum.hpp"

namespace chatterino {
enum class SeventvEventApiSubscriptionType {
    UpdateEmoteSet,
    UpdateUser,

    INVALID,
};

enum class SeventvEventApiOpcode {
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

enum class SeventvEventApiCloseCode {
    ServerError = 4000,
    UnknownOperation = 4001,
    InvalidPayload = 4002,
    AuthFailure = 4003,
    RateLimited = 4005,
    Restart = 4006,
    Maintenance = 4007,
    Timeout = 4008,
    AlreadySubscribed = 4009,
    NotSubscribed = 4010,
    InsufficientPrivilege = 4011,
};

struct SeventvEventApiSubscription {
    bool operator==(const SeventvEventApiSubscription &rhs) const;
    bool operator!=(const SeventvEventApiSubscription &rhs) const;
    QString condition;
    SeventvEventApiSubscriptionType type;
};
}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::SeventvEventApiSubscriptionType>(
    chatterino::SeventvEventApiSubscriptionType value) noexcept
{
    switch (value)
    {
        case chatterino::SeventvEventApiSubscriptionType::UpdateEmoteSet:
            return "emote_set.update";
        case chatterino::SeventvEventApiSubscriptionType::UpdateUser:
            return "user.update";

        default:
            return default_tag;
    }
}

namespace std {
template <>
struct hash<chatterino::SeventvEventApiSubscription> {
    size_t operator()(const chatterino::SeventvEventApiSubscription &sub) const
    {
        return (size_t)qHash(sub.condition, qHash((int)sub.type));
    }
};
}  // namespace std
