#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <rapidjson/document.h>
#include <QString>
#include <pajlada/serialize.hpp>

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

template <>
struct Deserialize<chatterino::TwitchUser> {
    static chatterino::TwitchUser get(const rapidjson::Value &value,
                                      bool *error = nullptr)
    {
        using namespace chatterino;

        TwitchUser user;

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return user;
        }

        if (!rj::getSafe(value, "_id", user.id))
        {
            PAJLADA_REPORT_ERROR(error)
            return user;
        }

        if (!rj::getSafe(value, "name", user.name))
        {
            PAJLADA_REPORT_ERROR(error)
            return user;
        }

        if (!rj::getSafe(value, "display_name", user.displayName))
        {
            PAJLADA_REPORT_ERROR(error)
            return user;
        }

        return user;
    }
};

}  // namespace pajlada
