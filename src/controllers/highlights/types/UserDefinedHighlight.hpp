// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/SharedHighlight2.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize/common.hpp>
#include <QStringView>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

namespace chatterino {

struct UserDefinedHighlight : public SharedHighlight2 {
    UserDefinedHighlight(QStringView _id)
        : id(_id)
    {
    }

    QString getName() const
    {
        return "User Defined: " % this->id;
    }

private:
    /// Unique identifier for this highlight.
    /// This should be a random UUID
    QString id;

    template <typename Type, typename RJValue>
    friend struct pajlada::Serialize;

    template <typename Type, typename RJValue, typename Enable>
    friend struct pajlada::Deserialize;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::UserDefinedHighlight> {
    using H = chatterino::UserDefinedHighlight;

    static rapidjson::Value get(const H &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);
        value.serialize(ret, a);
        chatterino::rj::set(ret, "id", value.id, a);
        return ret;
    }
};

template <>
struct Deserialize<chatterino::UserDefinedHighlight> {
    using H = chatterino::UserDefinedHighlight;

    static H get(const rapidjson::Value &value, bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return {u"invalid"};
        }

        // Because UserDefinedHighlight matches any highlight with an "id" field, it means this highlight must always attempt to be deserialized last

        QString id;
        if (!chatterino::rj::getSafe(value, "id", id))
        {
            PAJLADA_REPORT_ERROR(error)
            return {u"invalid"};
        }

        H h(id);

        if (!h.deserialize(value))
        {
            PAJLADA_REPORT_ERROR(error)
            return {u"invalid"};
        }

        return h;
    }
};

}  // namespace pajlada
