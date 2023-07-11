#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QHashFunctions>
#include <QString>
#include <rapidjson/document.h>

#include <cassert>

namespace chatterino {

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#    define QHashValue uint
#else
#    define QHashValue size_t
#endif

struct HelixBlock;

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

    void fromHelixBlock(const HelixBlock &ignore);

    bool operator<(const TwitchUser &rhs) const
    {
        return this->id < rhs.id;
    }

    bool operator==(const TwitchUser &rhs) const
    {
        return this->id == rhs.id;
    }

    bool operator!=(const TwitchUser &rhs) const
    {
        return !(*this == rhs);
    }

    friend QHashValue qHash(const TwitchUser &user, QHashValue seed) noexcept;
};

inline QHashValue qHash(const chatterino::TwitchUser &user,
                        QHashValue seed = 0) noexcept
{
    return qHash(user.id, seed);
}

#undef QHashValue

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
