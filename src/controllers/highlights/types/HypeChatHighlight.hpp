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

struct HypeChatHighlight {
    static constexpr QStringView ID = u"hypechat";
    static constexpr QStringView ICON_RESOURCE =
        u":/buttons/settings-darkMode.svg";

    static constexpr QStringView DEFAULT_NAME = u"Hype Chats";

    static constexpr bool ENABLED_BY_DEFAULT = true;
    static constexpr bool SHOW_IN_MENTIONS_DEFAULT = false;
    // TODO: does not support show in mentions
    static constexpr bool SUPPORTS_SHOW_IN_MENTIONS = false;
    static constexpr bool ALERT_DEFAULT = false;
    // TODO: Should we disable setting of the "highlight taskbar" somehow cuz it's not supported?
    static constexpr bool SUPPORT_ALERT = false;
    static constexpr bool PLAY_SOUND_DEFAULT = false;

    // Default state:
    // Enabled = true
    // Show in mentions = unavailable (always false)
    // Flash taskbar = unavailable (always false)
    // Play sound = false

    QString name;
    std::optional<bool> enabled;

    Outcome outcome;

    HighlightCheck buildCheck() const;
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::HypeChatHighlight> {
    using H = chatterino::highlights::HypeChatHighlight;

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
struct Deserialize<chatterino::highlights::HypeChatHighlight> {
    using H = chatterino::highlights::HypeChatHighlight;

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
