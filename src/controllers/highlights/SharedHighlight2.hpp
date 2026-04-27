// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <pajlada/serialize/common.hpp>
#include <pajlada/serialize/deserialize.hpp>
#include <pajlada/serialize/serialize.hpp>
#include <QColor>
#include <QDebug>
#include <QPixmap>
#include <QRegularExpression>
#include <QString>
#include <QStringView>
#include <QUrl>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <memory>
#include <optional>

namespace chatterino {

struct HighlightCheck;

struct SharedHighlight2 {
    bool operator==(const SharedHighlight2 &other) const = default;

    SharedHighlight2() = default;
    virtual ~SharedHighlight2() = default;

    /// Returns true if this highlight should be enabled.
    ///
    /// If unconfigured, returns true.
    virtual bool isEnabled() const;
    void setEnabled(std::optional<bool> newValue);

    /// Returns true if this highlight should show the message in /mentions.
    ///
    /// If unconfigured, returns true.
    virtual bool shouldShowInMentions() const;
    virtual void setShowInMentions(std::optional<bool> newValue);

    /// Returns true if this highlight should highlight the taskbar.
    ///
    /// If unconfigured, returns true.
    virtual bool shouldHighlightTaskbar() const;
    virtual void setHighlightTaskbar(std::optional<bool> newValue);

    /// Returns true if this highlight should play a sound.
    ///
    /// If unconfigured, returns false.
    virtual bool shouldPlaySound() const;
    void setPlaySound(std::optional<bool> newValue);

    QUrl getSoundUrl() const;
    void setSoundUrl(const QUrl &newValue);

    /// Unique identifier for this highlight
    /// User-created highlights will get a random UUID,
    /// while app-created highlights get a descriptive string (i.e. "subhighlight")
    // TODO: Move to custom highlight or something
    // QString id;

    /// The display name/pretty name of this highlight.
    /// If empty, we will try to auto-generate something that makes sense (e.g. "Text contains 'foo'")
    QString name;

protected:
    std::optional<bool> enabled;

    static bool matchesID(const rapidjson::Value &value, QStringView expectedID)
    {
        QString id;
        chatterino::rj::getSafe(value, "id", id);
        return id == expectedID;
    }

public:
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

protected:
    /// Whether to add the matching message to the /mentions channel
    std::optional<bool> showInMentions;

    /// Show an OS-specific alert.
    /// On Windows, this will flash Chatterino in the taskbar.
    /// On macOS, this will make Chatterino bounce in the taskbar.
    std::optional<bool> alert;

    /// Play a sound.
    /// If the highlight specifies a "customSoundURL", it will play that, otherwise it will
    /// play the default highlight sound.
    std::optional<bool> playSound;

    /// The custom sound URL to play if playSound is enabled.
    QUrl customSoundURL;

public:
    /// The background color to apply to the message.
    /// If the color is invalid/unset, don't apply a background color.
    std::shared_ptr<QColor> backgroundColor = std::make_shared<QColor>();

    std::optional<bool> isRegex;
    std::optional<bool> isCaseSensitive;

    QPixmap getType() const;

    bool willPlayAnySound() const;
    bool willPlayCustomSound() const;

    HighlightCheck buildCheck() const;

protected:
    void serialize(rapidjson::Value &ret,
                   rapidjson::Document::AllocatorType &a) const
    {
        // error if id is not set, or use "invalid" or something
        // TODO: id should probably be set in some CustomHighlight struct instead
        // rj::set(ret, "id", this->id, a);

        rj::setOptionally(ret, "name", this->name, a);
        rj::setOptionally(ret, "enabled", this->enabled, a);
        rj::setOptionally(ret, "pattern", this->pattern, a);
        rj::setOptionally(ret, "showInMentions", this->showInMentions, a);
        rj::setOptionally(ret, "alert", this->alert, a);
        rj::setOptionally(ret, "playSound", this->playSound, a);
        if (!this->customSoundURL.isEmpty())
        {
            rj::set(ret, "customSoundURL", this->customSoundURL.toString(), a);
        }

        if (this->backgroundColor->isValid())
        {
            rj::set(ret, "backgroundColor",
                    this->backgroundColor->name(QColor::HexArgb), a);
        }

        rj::setOptionally(ret, "regex", this->isRegex, a);
        rj::setOptionally(ret, "caseSensitive", this->isCaseSensitive, a);
    }

    bool deserialize(const rapidjson::Value &value)
    {
        assert(value.IsObject());

        QColor tmpBackgroundColor;

        chatterino::rj::getSafe(value, "name", this->name);
        chatterino::rj::getSafe(value, "enabled", this->enabled);
        chatterino::rj::getSafe(value, "pattern", this->pattern);
        chatterino::rj::getSafe(value, "showInMentions", this->showInMentions);
        chatterino::rj::getSafe(value, "alert", this->alert);
        chatterino::rj::getSafe(value, "playSound", this->playSound);
        chatterino::rj::getSafe(value, "customSoundURL", this->customSoundURL);
        chatterino::rj::getSafe(value, "backgroundColor", tmpBackgroundColor);
        chatterino::rj::getSafe(value, "regex", this->isRegex);
        chatterino::rj::getSafe(value, "caseSensitive", this->isCaseSensitive);

        this->backgroundColor = std::make_shared<QColor>(tmpBackgroundColor);

        return true;
    }

    template <typename Type, typename RJValue>
    friend struct pajlada::Serialize;

    template <typename Type, typename RJValue, typename Enable>
    friend struct pajlada::Deserialize;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::SharedHighlight2> {
    static rapidjson::Value get(const chatterino::SharedHighlight2 &value,
                                rapidjson::Document::AllocatorType &a)
    {
        using namespace chatterino;
        rapidjson::Value ret(rapidjson::kObjectType);
        value.serialize(ret, a);
        return ret;
    }
};

template <>
struct Deserialize<chatterino::SharedHighlight2> {
    static chatterino::SharedHighlight2 get(const rapidjson::Value &value,
                                            bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        chatterino::SharedHighlight2 h;

        if (!h.deserialize(value))
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        return h;
    }
};

}  // namespace pajlada

// TODO: Confirm this is necessary for a stupid type
Q_DECLARE_METATYPE(chatterino::SharedHighlight2);

QDebug operator<<(QDebug dbg, const chatterino::SharedHighlight2 &v);
