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
#include <QDebug>
#include <QStringView>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

namespace chatterino {

struct HighlightCheck;

}  // namespace chatterino

namespace chatterino::highlights {

/// Matches Twitch announcements
///
/// Can be further customized to override the colors of colored announcements
struct InvalidHighlight {
    static constexpr QStringView ID = u"invalid";
    static constexpr QStringView ICON_RESOURCE = u":/buttons/cancel.svg";

    static constexpr QStringView DEFAULT_NAME = u"Invalid";

    static constexpr bool ENABLED_BY_DEFAULT = false;
    static constexpr bool SHOW_IN_MENTIONS_DEFAULT = false;
    static constexpr bool ALERT_DEFAULT = false;
    static constexpr QColor BACKGROUND_COLOR_DEFAULT = QColor(0, 0, 0, 0);

    Outcome outcome{BACKGROUND_COLOR_DEFAULT};

    bool isValid();

    HighlightCheck buildCheck() const;

    friend QDebug operator<<(QDebug dbg, const InvalidHighlight &v);
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::InvalidHighlight> {
    using H = chatterino::highlights::InvalidHighlight;

    static rapidjson::Value get(const H &h,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);
        chatterino::rj::set(ret, "id", H::ID, a);

        h.outcome.serialize(ret, a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::highlights::InvalidHighlight> {
    using H = chatterino::highlights::InvalidHighlight;

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

        h.outcome.deserialize(value);

        return h;
    }
};

}  // namespace pajlada
