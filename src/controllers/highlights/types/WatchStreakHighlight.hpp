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

#include <cassert>
#include <optional>

namespace chatterino {

struct WatchStreakHighlight : public SharedHighlight2 {
    static constexpr QStringView ID = u"watchstreak";

    WatchStreakHighlight() = default;

    // Default state:
    // Enabled = true
    // Show in mentions = unavailable (always false)
    // Flash taskbar = unavailable (always false)
    // Play sound = false

    bool shouldShowInMentions() const override
    {
        return false;
    }

    void setShowInMentions(std::optional<bool> newValue) override
    {
        (void)newValue;
        assert(false && "WatchStreak do not support 'show in mentions'");
    }

    bool shouldHighlightTaskbar() const override
    {
        return false;
    }

    void setHighlightTaskbar(std::optional<bool> newValue) override
    {
        (void)newValue;
        assert(false && "WatchStreak do not support 'flash taskbar'");
    }
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::WatchStreakHighlight> {
    using H = chatterino::WatchStreakHighlight;

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
struct Deserialize<chatterino::WatchStreakHighlight> {
    using H = chatterino::WatchStreakHighlight;

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
