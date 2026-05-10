// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/SharedHighlight2.hpp"
#include "controllers/highlights/types/Common.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize/common.hpp>
#include <QStringView>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <optional>

namespace chatterino::highlights {

struct WhispersHighlight : public SharedHighlight2 {
    static constexpr QStringView ID = u"whispers";

    WhispersHighlight() = default;

    QString getDefaultName() const
    {
        return "Whispers";
    }

    QString getName() const
    {
        if (this->name.isEmpty())
        {
            return this->getDefaultName();
        }
        return this->name;
    }

    QStringView getID() const
    {
        return ID;
    }

    // Default state:
    // Enabled = true
    // Show in mentions = unavailable (always false)
    // Flash taskbar = false
    // Play sound = false

    bool shouldShowInMentions() const override
    {
        return false;
    }

    void setShowInMentions(std::optional<bool> newValue) override
    {
        (void)newValue;
        assert(false && "Whispers do not support 'show in mentions'");
    }

    bool shouldHighlightTaskbar() const override
    {
        return this->outcome.alert.value_or(false);
    }

    HighlightCheck buildCheck() const;
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::WhispersHighlight> {
    using H = chatterino::highlights::WhispersHighlight;

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
struct Deserialize<chatterino::highlights::WhispersHighlight> {
    using H = chatterino::highlights::WhispersHighlight;

    static H get(const rapidjson::Value &value, bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        if (!chatterino::highlights::matchesID(value, H::ID))
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
