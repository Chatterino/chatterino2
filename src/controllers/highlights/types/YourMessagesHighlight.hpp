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

#include <optional>

namespace chatterino {

struct HighlightCheck;

}  // namespace chatterino

namespace chatterino::highlights {

struct YourMessagesHighlight {
    static constexpr QStringView ID = u"yourmessages";
    static constexpr QStringView ICON_RESOURCE =
        u":/buttons/settings-darkMode.svg";

    // TODO: Remove (automatic) portion of the default name?
    static constexpr QStringView DEFAULT_NAME = u"Your messages (automatic)";

    static constexpr bool ENABLED_BY_DEFAULT = false;
    static constexpr bool SHOW_IN_MENTIONS_DEFAULT = false;
    static constexpr bool ALERT_DEFAULT = false;
    static constexpr bool SUPPORT_ALERT = false;
    static constexpr bool PLAY_SOUND_DEFAULT = false;
    static constexpr bool SUPPORT_PLAY_SOUND = false;

    // Default state:
    // Enabled = false
    // Show in mentions = false
    // Flash taskbar = false (not possible to enable)
    // Play sound = false (not possible to configure)

    QString name;
    std::optional<bool> enabled;

    Outcome outcome;

    HighlightCheck buildCheck() const;
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::YourMessagesHighlight> {
    using H = chatterino::highlights::YourMessagesHighlight;

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
struct Deserialize<chatterino::highlights::YourMessagesHighlight> {
    using H = chatterino::highlights::YourMessagesHighlight;

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
