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

namespace chatterino::highlights {

struct YourUsernameHighlight : public SharedHighlight2 {
    static constexpr QStringView ID = u"yourusername";

    YourUsernameHighlight() = default;

    QString getDefaultName() const
    {
        // TODO: remove automatic portion of name?
        return "Your Username (automatic)";
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
    // Show in mentions = true
    // Flash taskbar = true
    // Play sound = true

    bool shouldPlaySound() const override
    {
        return this->outcome.playSound.value_or(true);
    }

    HighlightCheck buildCheck() const;
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::YourUsernameHighlight> {
    using H = chatterino::highlights::YourUsernameHighlight;

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
struct Deserialize<chatterino::highlights::YourUsernameHighlight> {
    using H = chatterino::highlights::YourUsernameHighlight;

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
