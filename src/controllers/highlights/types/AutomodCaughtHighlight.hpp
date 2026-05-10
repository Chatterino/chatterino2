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

namespace chatterino::highlights {

struct AutomodCaughtHighlight : public SharedHighlight2 {
    static constexpr QStringView ID = u"automodcaught";

    AutomodCaughtHighlight() = default;

    QString getDefaultName() const
    {
        return "AutoMod Caught Messages";
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
    // Show in mentions = false
    // Flash taskbar = false
    // Play sound = false

    bool shouldShowInMentions() const override
    {
        return this->outcome.showInMentions.value_or(false);
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
struct Serialize<chatterino::highlights::AutomodCaughtHighlight> {
    using H = chatterino::highlights::AutomodCaughtHighlight;

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
struct Deserialize<chatterino::highlights::AutomodCaughtHighlight> {
    using H = chatterino::highlights::AutomodCaughtHighlight;

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
