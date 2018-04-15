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
    PubSubAction(const rapidjson::Value &data);
    ActionUser source;

    std::chrono::steady_clock::time_point timestamp;
};

// Used when a chat mode (i.e. slowmode, subscribers only mode) is enabled or disabled
struct ModeChangedAction : PubSubAction {
    ModeChangedAction(const rapidjson::Value &data)
        : PubSubAction(data)
    {
    }

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

struct TimeoutAction : PubSubAction {
    TimeoutAction(const rapidjson::Value &data)
        : PubSubAction(data)
    {
    }

    ActionUser target;

    QString reason;
    uint32_t duration;
};

struct BanAction : PubSubAction {
    BanAction(const rapidjson::Value &data)
        : PubSubAction(data)
    {
    }

    ActionUser target;

    QString reason;
};

struct UnbanAction : PubSubAction {
    UnbanAction(const rapidjson::Value &data)
        : PubSubAction(data)
    {
    }

    ActionUser target;

    enum {
        Banned,
        TimedOut,
    } previousState;
};

struct ClearChatAction : PubSubAction {
    ClearChatAction(const rapidjson::Value &data)
        : PubSubAction(data)
    {
    }
};

struct ModerationStateAction : PubSubAction {
    ModerationStateAction(const rapidjson::Value &data)
        : PubSubAction(data)
    {
    }

    ActionUser target;

    // true = modded
    // false = unmodded
    bool modded;
};

}  // namespace singletons
}  // namespace chatterino
