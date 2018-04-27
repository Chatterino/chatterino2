#pragma once

#include <rapidjson/document.h>

#include <QString>

#include <chrono>
#include <cinttypes>

namespace chatterino {
namespace singletons {

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

// Used when a chat mode (i.e. slowmode, subscribers only mode) is enabled or disabled
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

    union {
        uint32_t duration;
    } args;
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

}  // namespace singletons
}  // namespace chatterino
