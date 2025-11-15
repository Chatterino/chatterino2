#pragma once

#include "pajlada/serialize/common.hpp"
#include "pajlada/serialize/deserialize.hpp"
#include "pajlada/serialize/serialize.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QColor>
#include <QDebug>
#include <QPixmap>
#include <QRegularExpression>
#include <QString>
#include <QUrl>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <memory>

namespace chatterino {

struct HighlightCheck;

struct SharedHighlight {
    bool operator==(const SharedHighlight &other) const = default;

    /// Unique identifier for this highlight
    /// User-created highlights will get a random UUID,
    /// while app-created highlights get a descriptive string (i.e. "subhighlight")
    QString id;

    /// The display name/pretty name of this highlight.
    /// If empty, we will try to auto-generate something that makes sense (e.g. "Text contains 'foo'")
    QString name;

    bool enabled = false;

    /// Contains the highlight pattern.
    /// This can be a simple text match:
    ///  - "foo" would match if the message text contains "foo"
    ///  - "regex:^foo$" would match if the message text is exactly "foo"
    ///  - "cs:FoO" would match if the message text contains "FoO" (case sensitive)
    /// meta matchers
    ///  - "badge:sub" would match if the author has any sub badge
    ///  - "name:forsen" would match if the author's name is forsen
    /// or any combination of the above
    ///  - "badge:sub foo" would match if the user has any sub badge and their message text contains "foo"
    QString pattern;

    /// Whether to add the matching message to the /mentions channel
    bool showInMentions = false;

    /// Show an OS-specific alert.
    /// On Windows, this will flash Chatterino in the taskbar.
    /// On macOS, this will make Chatterino bounce in the taskbar.
    bool alert = false;

    /// Play a sound.
    /// If the highlight specifies a "customSoundURL", it will play that, otherwise it will
    /// play the default highlight sound.
    bool playSound = false;

    /// The custom sound URL to play if playSound is enabled.
    QUrl customSoundURL;

    /// The background color to apply to the message.
    /// If the color is invalid/unset, don't apply a background color.
    std::shared_ptr<QColor> backgroundColor = std::make_shared<QColor>();

    bool isRegex = false;
    bool isCaseSensitive = false;

    QPixmap getType() const;

    bool willPlayAnySound() const;
    bool willPlayCustomSound() const;

    HighlightCheck buildCheck() const;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::SharedHighlight> {
    static rapidjson::Value get(const chatterino::SharedHighlight &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        assert(value.backgroundColor);

        chatterino::rj::set(ret, "id", value.id, a);
        chatterino::rj::set(ret, "name", value.name, a);
        chatterino::rj::set(ret, "enabled", value.enabled, a);
        chatterino::rj::set(ret, "pattern", value.pattern, a);
        chatterino::rj::set(ret, "showInMentions", value.showInMentions, a);
        chatterino::rj::set(ret, "alert", value.alert, a);
        chatterino::rj::set(ret, "playSound", value.playSound, a);
        chatterino::rj::set(ret, "customSoundURL",
                            value.customSoundURL.toString(), a);

        chatterino::rj::set(ret, "backgroundColor",
                            value.backgroundColor->name(QColor::HexArgb), a);

        chatterino::rj::set(ret, "regex", value.isRegex, a);
        chatterino::rj::set(ret, "caseSensitive", value.isCaseSensitive, a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::SharedHighlight> {
    static chatterino::SharedHighlight get(const rapidjson::Value &value,
                                           bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)

            return {
                // TODO: default error state?
            };
        }

        chatterino::SharedHighlight h;

        QColor backgroundColor;

        chatterino::rj::getSafe(value, "id", h.id);
        chatterino::rj::getSafe(value, "name", h.name);
        chatterino::rj::getSafe(value, "enabled", h.enabled);
        chatterino::rj::getSafe(value, "pattern", h.pattern);
        chatterino::rj::getSafe(value, "showInMentions", h.showInMentions);
        chatterino::rj::getSafe(value, "alert", h.alert);
        chatterino::rj::getSafe(value, "playSound", h.playSound);
        chatterino::rj::getSafe(value, "customSoundURL", h.customSoundURL);
        chatterino::rj::getSafe(value, "backgroundColor", backgroundColor);
        chatterino::rj::getSafe(value, "regex", h.isRegex);
        chatterino::rj::getSafe(value, "caseSensitive", h.isCaseSensitive);

        h.backgroundColor = std::make_shared<QColor>(backgroundColor);

        return h;
    }
};

}  // namespace pajlada

// TODO: Confirm this is necessary for a stupid type
Q_DECLARE_METATYPE(chatterino::SharedHighlight);

QDebug operator<<(QDebug dbg, const chatterino::SharedHighlight &v);
