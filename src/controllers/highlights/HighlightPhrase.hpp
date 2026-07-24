// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "pajlada/serialize/common.hpp"
#include "pajlada/serialize/deserialize.hpp"
#include "pajlada/serialize/serialize.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QColor>
#include <QString>
#include <QUrl>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

namespace chatterino {

/// HighlightPhrase is how the old highlight system defined a message and user highlight
/// The base struct & serialization is kept for migration from old settings versions.
struct HighlightPhrase {
    static constexpr QColor FALLBACK_HIGHLIGHT_COLOR = QColor(127, 63, 73, 127);
    /// Used for automatic self messages highlighing
    static constexpr QColor FALLBACK_SELF_MESSAGE_HIGHLIGHT_COLOR =
        QColor(0, 118, 221, 115);
    static constexpr QColor FALLBACK_REDEEMED_HIGHLIGHT_COLOR =
        QColor(28, 126, 141, 60);
    static constexpr QColor FALLBACK_FIRST_MESSAGE_HIGHLIGHT_COLOR =
        QColor(72, 127, 63, 60);
    static constexpr QColor FALLBACK_ELEVATED_MESSAGE_HIGHLIGHT_COLOR =
        QColor(255, 174, 66, 60);
    static constexpr QColor FALLBACK_THREAD_HIGHLIGHT_COLOR =
        QColor(143, 48, 24, 60);
    static constexpr QColor FALLBACK_ANNOUNCEMENT_HIGHLIGHT_COLOR =
        QColor(255, 102, 237, 100);
    static constexpr QColor ANNOUNCEMENT_BLUE_HIGHLIGHT_COLOR =
        QColor(102, 148, 255, 100);
    static constexpr QColor ANNOUNCEMENT_GREEN_HIGHLIGHT_COLOR =
        QColor(96, 255, 96, 100);
    static constexpr QColor ANNOUNCEMENT_ORANGE_HIGHLIGHT_COLOR =
        QColor(233, 210, 0, 100);
    static constexpr QColor ANNOUNCEMENT_PURPLE_HIGHLIGHT_COLOR =
        QColor(255, 102, 237, 100);

    QString pattern;
    bool showInMentions{false};
    bool hasAlert{false};
    bool hasSound{false};
    bool isRegex{false};
    bool isCaseSensitive{false};
    QUrl soundUrl;
    QColor color;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::HighlightPhrase> {
    static rapidjson::Value get(const chatterino::HighlightPhrase &value,
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
struct Deserialize<chatterino::HighlightPhrase> {
    static chatterino::HighlightPhrase get(const rapidjson::Value &value,
                                           bool *error = nullptr)
    {
        chatterino::HighlightPhrase h;
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
            iColor = chatterino::HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR;
        }
        else
        {
            h.color = iColor;
        }

        return h;
    }
};

}  // namespace pajlada
