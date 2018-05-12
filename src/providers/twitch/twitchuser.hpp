#pragma once

#include <rapidjson/document.h>
#include <QString>

#include <cassert>

namespace chatterino {
namespace providers {
namespace twitch {

struct TwitchUser {
    QString id;
    mutable QString name;
    mutable QString displayName;

    void update(const TwitchUser &other) const
    {
        assert(this->id == other.id);

        this->name = other.name;
        this->displayName = other.displayName;
    }

    static TwitchUser fromJSON(const rapidjson::Value &value);

    bool operator<(const TwitchUser &rhs) const
    {
        return this->id < rhs.id;
    }
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
