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

struct YourUsernameHighlight : public SharedHighlight2 {
    static constexpr QStringView ID = u"yourusername";

    YourUsernameHighlight() = default;

    // Default state:
    // Enabled = true
    // Show in mentions = true
    // Flash taskbar = true
    // Play sound = true

    bool shouldPlaySound() const override
    {
        return this->playSound.value_or(true);
    }
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::YourUsernameHighlight> {
    using H = chatterino::YourUsernameHighlight;

    static rapidjson::Value get(const H &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);
        value.serialize(ret, a);
        chatterino::rj::set(ret, "id", H::ID, a);
        return ret;
    }
};

template <>
struct Deserialize<chatterino::YourUsernameHighlight> {
    using H = chatterino::YourUsernameHighlight;

    static H get(const rapidjson::Value &value, bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        if (!H::matchesID(value, H::ID))
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        H h;

        if (!h.deserialize(value))
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        return h;
    }
};

}  // namespace pajlada
