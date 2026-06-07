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

struct YourMessagesHighlight : public SharedHighlight2 {
    static constexpr QStringView ID = u"yourmessages";

    YourMessagesHighlight() = default;

    QString getName() const
    {
        return "Your messages (automatic)";
    }

    // Default state:
    // Enabled = false
    // Show in mentions = false
    // Flash taskbar = false (not possible to enable)
    // Play sound = false (not possible to configure)

    bool isEnabled() const override
    {
        return this->enabled.value_or(false);
    }

    bool shouldShowInMentions() const override
    {
        return this->showInMentions.value_or(false);
    }

    bool shouldHighlightTaskbar() const override
    {
        return false;
    }

    bool shouldPlaySound() const override
    {
        return false;
    }

    HighlightCheck buildCheck() const;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::YourMessagesHighlight> {
    using H = chatterino::YourMessagesHighlight;

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
struct Deserialize<chatterino::YourMessagesHighlight> {
    using H = chatterino::YourMessagesHighlight;

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
