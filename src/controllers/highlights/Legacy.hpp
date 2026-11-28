// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/types/MessageHighlight.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"  // IWYU pragma: keep

#include <pajlada/serialize.hpp>
#include <pajlada/serialize/common.hpp>
#include <pajlada/serialize/deserialize.hpp>
#include <pajlada/serialize/serialize.hpp>
#include <QColor>
#include <QString>
#include <QUrl>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

namespace chatterino::highlights::legacy {

/// HighlightBadge is how the old highlight system defined a badge highlight
/// The base struct & serialization is kept for migration from old settings versions.
struct HighlightBadge {
    static constexpr QColor FALLBACK_HIGHLIGHT_COLOR = QColor(127, 63, 73, 127);

    QString badgeName;
    QString displayName;
    bool showInMentions{false};
    bool hasAlert{false};
    bool hasSound{false};
    QUrl soundUrl;
    QColor color;
};

/// HighlightPhrase is how the old highlight system defined a message and user highlight
/// The base struct & serialization is kept for migration from old settings versions.
struct HighlightPhrase {
    QString pattern;
    bool showInMentions{false};
    bool hasAlert{false};
    bool hasSound{false};
    bool isRegex{false};
    bool isCaseSensitive{false};
    QUrl soundUrl;
    QColor color;
};

};  // namespace chatterino::highlights::legacy

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::legacy::HighlightBadge> {
    static rapidjson::Value get(
        const chatterino::highlights::legacy::HighlightBadge &value,
        rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value.badgeName, a);
        chatterino::rj::set(ret, "displayName", value.displayName, a);
        chatterino::rj::set(ret, "showInMentions", value.showInMentions, a);
        chatterino::rj::set(ret, "alert", value.hasAlert, a);
        chatterino::rj::set(ret, "sound", value.hasSound, a);
        chatterino::rj::set(ret, "soundUrl", value.soundUrl.toString(), a);
        chatterino::rj::set(ret, "color", value.color.name(QColor::HexArgb), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::highlights::legacy::HighlightBadge> {
    static chatterino::highlights::legacy::HighlightBadge get(
        const rapidjson::Value &value, bool *error)
    {
        chatterino::highlights::legacy::HighlightBadge h;

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return h;
        }

        QString iSoundUrl;
        QString encodedColor;

        chatterino::rj::getSafe(value, "name", h.badgeName);
        chatterino::rj::getSafe(value, "displayName", h.displayName);
        chatterino::rj::getSafe(value, "showInMentions", h.showInMentions);
        chatterino::rj::getSafe(value, "alert", h.hasAlert);
        chatterino::rj::getSafe(value, "sound", h.hasSound);
        chatterino::rj::getSafe(value, "soundUrl", iSoundUrl);
        h.soundUrl = iSoundUrl;
        chatterino::rj::getSafe(value, "color", encodedColor);

        auto iColor = QColor(encodedColor);
        if (!iColor.isValid())
        {
            iColor = chatterino::highlights::legacy::HighlightBadge::
                FALLBACK_HIGHLIGHT_COLOR;
        }
        else
        {
            h.color = iColor;
        }

        return h;
    }
};

template <>
struct Serialize<chatterino::highlights::legacy::HighlightPhrase> {
    static rapidjson::Value get(
        const chatterino::highlights::legacy::HighlightPhrase &value,
        rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "pattern", value.pattern, a);
        chatterino::rj::set(ret, "showInMentions", value.showInMentions, a);
        chatterino::rj::set(ret, "alert", value.hasAlert, a);
        chatterino::rj::set(ret, "sound", value.hasSound, a);
        chatterino::rj::set(ret, "regex", value.isRegex, a);
        chatterino::rj::set(ret, "case", value.isCaseSensitive, a);
        chatterino::rj::set(ret, "soundUrl", value.soundUrl.toString(), a);
        chatterino::rj::set(ret, "color", value.color.name(QColor::HexArgb), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::highlights::legacy::HighlightPhrase> {
    static chatterino::highlights::legacy::HighlightPhrase get(
        const rapidjson::Value &value, bool *error = nullptr)
    {
        chatterino::highlights::legacy::HighlightPhrase h;
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)

            return h;
        }

        QString iSoundUrl;
        QString encodedColor;

        chatterino::rj::getSafe(value, "pattern", h.pattern);
        chatterino::rj::getSafe(value, "showInMentions", h.showInMentions);
        chatterino::rj::getSafe(value, "alert", h.hasAlert);
        chatterino::rj::getSafe(value, "sound", h.hasSound);
        chatterino::rj::getSafe(value, "regex", h.isRegex);
        chatterino::rj::getSafe(value, "case", h.isCaseSensitive);
        chatterino::rj::getSafe(value, "soundUrl", iSoundUrl);
        h.soundUrl = iSoundUrl;
        chatterino::rj::getSafe(value, "color", encodedColor);

        auto iColor = QColor(encodedColor);
        if (!iColor.isValid())
        {
            iColor = chatterino::highlights::MessageHighlight::
                BACKGROUND_COLOR_DEFAULT;
        }
        else
        {
            h.color = iColor;
        }

        return h;
    }
};

}  // namespace pajlada
