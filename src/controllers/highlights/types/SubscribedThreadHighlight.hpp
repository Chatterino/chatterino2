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

struct SubscribedThreadHighlight : public SharedHighlight2 {
    static constexpr QStringView ID = u"subscribedthread";

    SubscribedThreadHighlight() = default;

    QString getDefaultName() const
    {
        return "Subscribed Reply Threads";
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
struct Serialize<chatterino::highlights::SubscribedThreadHighlight> {
    using H = chatterino::highlights::SubscribedThreadHighlight;

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
struct Deserialize<chatterino::highlights::SubscribedThreadHighlight> {
    using H = chatterino::highlights::SubscribedThreadHighlight;

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
