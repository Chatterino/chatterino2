// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/types/Common.hpp"
#include "controllers/highlights/types/Outcome.hpp"
#include "pajlada/serialize/deserialize.hpp"
#include "pajlada/serialize/serialize.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize/common.hpp>
#include <QStringView>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <optional>

namespace chatterino {

struct HighlightCheck;

}  // namespace chatterino

namespace chatterino::highlights {

struct SubscribedThreadHighlight {
    static constexpr QStringView ID = u"subscribedthread";
    static constexpr QStringView ICON_RESOURCE =
        u":/buttons/settings-darkMode.svg";

    static constexpr QStringView DEFAULT_NAME = u"Subscribed Reply Threads";

    static constexpr bool ENABLED_BY_DEFAULT = true;
    static constexpr bool SHOW_IN_MENTIONS_DEFAULT = true;
    static constexpr bool ALERT_DEFAULT = true;
    static constexpr bool PLAY_SOUND_DEFAULT = true;

    // Default state:
    // Enabled = true
    // Show in mentions = true
    // Flash taskbar = true
    // Play sound = true

    QString name;
    std::optional<bool> enabled;

    Outcome outcome;

    HighlightCheck buildCheck() const;
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::SubscribedThreadHighlight> {
    using H = chatterino::highlights::SubscribedThreadHighlight;

    static rapidjson::Value get(const H &h,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);
        chatterino::rj::set(ret, "id", H::ID, a);

        chatterino::rj::setOptionally(ret, "name", h.name, a);
        chatterino::rj::setOptionally(ret, "enabled", h.enabled, a);

        h.outcome.serialize(ret, a);

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

        if (!chatterino::highlights::matchesID(value, H::ID))
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        H h;

        chatterino::rj::getSafe(value, "name", h.name);
        chatterino::rj::getSafe(value, "enabled", h.enabled);

        h.outcome.deserialize(value);

        return h;
    }
};

}  // namespace pajlada
