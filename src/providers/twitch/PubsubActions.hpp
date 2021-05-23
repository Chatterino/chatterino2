#pragma once

#include <rapidjson/document.h>
#include <QString>

#include <chrono>
#include <cinttypes>

namespace chatterino {

struct ActionUser {
    QString id;
    QString name;
};

struct PubSubAction {
    PubSubAction(const rapidjson::Value &data, const QString &_roomID);
    ActionUser source;

    std::chrono::steady_clock::time_point timestamp;
    QString roomID;
};

// Used when a chat mode (i.e. slowmode, subscribers only mode) is enabled or
// disabled
struct ModeChangedAction : PubSubAction {
    using PubSubAction::PubSubAction;

    enum Mode {
        Unknown,
        Slow,
        R9K,
        SubscribersOnly,
        EmoteOnly,
    } mode;

    // Whether the mode was turned on or off
    enum State {
        Off,
        On,
    } state;

    uint32_t duration = 0;

    const char *getModeName() const
    {
        switch (this->mode)
        {
            case Mode::Slow:
                return "slow";
            case Mode::R9K:
                return "r9k";
            case Mode::SubscribersOnly:
                return "subscribers-only";
            case Mode::EmoteOnly:
                return "emote-only";
            default:
                return "unknown";
        }
    }
};

struct BanAction : PubSubAction {
    using PubSubAction::PubSubAction;

    ActionUser target;

    QString reason;

    uint32_t duration = 0;

    bool isBan() const
    {
        return this->duration == 0;
    }
};

struct UnbanAction : PubSubAction {
    using PubSubAction::PubSubAction;

    ActionUser target;

    enum {
        Banned,
        TimedOut,
    } previousState;

    bool wasBan() const
    {
        return this->previousState == Banned;
    }
};

struct ClearChatAction : PubSubAction {
    using PubSubAction::PubSubAction;
};

struct ModerationStateAction : PubSubAction {
    using PubSubAction::PubSubAction;

    ActionUser target;

    // true = modded
    // false = unmodded
    bool modded;
};

struct AutomodAction : PubSubAction {
    using PubSubAction::PubSubAction;

    ActionUser target;

    QString message;

    QString reason;

    QString msgID;
};

struct AutomodUserAction : PubSubAction {
    using PubSubAction::PubSubAction;

    ActionUser target;

    enum {
        AddPermitted,
        RemovePermitted,
        AddBlocked,
        RemoveBlocked,
        Properties,
    } type;

    QString message;
};

struct AutomodInfoAction : PubSubAction {
    using PubSubAction::PubSubAction;
    enum {
        OnHold,
        Denied,
        Approved,
    } type;
};

}  // namespace chatterino
