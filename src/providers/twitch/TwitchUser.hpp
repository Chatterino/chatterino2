#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <rapidjson/document.h>
#include <QString>
#include <pajlada/settings/serialize.hpp>

#include <cassert>

namespace chatterino {

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

    bool operator<(const TwitchUser &rhs) const
    {
        return this->id < rhs.id;
    }
};

}  // namespace chatterino

namespace pajlada {
namespace Settings {

template <>
struct Deserialize<chatterino::TwitchUser> {
    static chatterino::TwitchUser get(const rapidjson::Value &value, bool *error = nullptr)
    {
        using namespace chatterino;

        TwitchUser user;

        if (!value.IsObject()) {
            PAJLADA_REPORT_ERROR(error)
            PAJLADA_THROW_EXCEPTION("Deserialized rapidjson::Value is wrong type");
            return user;
        }

        if (!rj::getSafe(value, "_id", user.id)) {
            PAJLADA_REPORT_ERROR(error)
            PAJLADA_THROW_EXCEPTION("Missing ID key");
            return user;
        }

        if (!rj::getSafe(value, "name", user.name)) {
            PAJLADA_REPORT_ERROR(error)
            PAJLADA_THROW_EXCEPTION("Missing name key");
            return user;
        }

        if (!rj::getSafe(value, "display_name", user.displayName)) {
            PAJLADA_REPORT_ERROR(error)
            PAJLADA_THROW_EXCEPTION("Missing display name key");
            return user;
        }

        return user;
    }
};

}  // namespace Settings
}  // namespace pajlada
