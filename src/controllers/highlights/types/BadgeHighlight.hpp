// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/types/Common.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize/common.hpp>
#include <qcolor.h>
#include <qdebug.h>
#include <qpixmap.h>
#include <QStringList>
#include <QStringView>
#include <qurl.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <memory>
#include <optional>

namespace chatterino::highlights {

struct BadgeHighlight {
    static constexpr QStringView TYPE = u"badge";

    BadgeHighlight(QStringView _id);

    QString getDefaultName() const
    {
        // TODO: This should have some cool icon too
        return this->displayName;
    }

    QString getName() const
    {
        if (this->name.isEmpty())
        {
            return this->getDefaultName();
        }
        return this->name;
    }

    void setDisplayName(const QString &newValue)
    {
        this->displayName = newValue;
    }

    QString displayName;

    QStringView getID() const
    {
        return this->id;
    }

    bool operator==(const BadgeHighlight &other) const = default;

    /// Returns true if this highlight should be enabled.
    ///
    /// If unconfigured, returns true.
    bool isEnabled() const;

    /// Returns true if this highlight should show the message in /mentions.
    ///
    /// If unconfigured, returns true.
    bool shouldShowInMentions() const;
    void setShowInMentions(std::optional<bool> newValue);

    /// Returns true if this highlight should highlight the taskbar.
    ///
    /// If unconfigured, returns true.
    bool shouldHighlightTaskbar() const;
    void setHighlightTaskbar(std::optional<bool> newValue);

    QString getBadgeName() const
    {
        return this->badgeName;
    }

    void setBadgeName(const QString &newValue)
    {
        this->badgeName = newValue;

        this->rebuildBadgeCheck();
    }

    /// Returns true if this highlight should play a sound.
    ///
    /// If unconfigured, returns false.
    bool shouldPlaySound() const;
    void setPlaySound(std::optional<bool> newValue);

    QUrl getSoundUrl() const;
    void setSoundUrl(const QUrl &newValue);

    std::shared_ptr<QColor> getBackgroundColor() const;
    void setBackgroundColor(const QColor &newValue);

    /// The display name/pretty name of this highlight.
    /// If empty, we will try to auto-generate something that makes sense (e.g. "Text contains 'foo'")
    QString name;

    void debug() const;

protected:
    std::optional<bool> enabled;

    /// Contains the raw badge name
    QString badgeName;

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

    /// The background color to apply to the message.
    /// If the color is invalid/unset, don't apply a background color.
    std::shared_ptr<QColor> backgroundColor = std::make_shared<QColor>();

public:
    bool willPlayAnySound() const;
    bool willPlayCustomSound() const;

    HighlightCheck buildCheck() const;

protected:
    template <typename Type, typename RJValue>
    friend struct pajlada::Serialize;

    template <typename Type, typename RJValue, typename Enable>
    friend struct pajlada::Deserialize;

    friend class ConfigureDialog;

    /// Unique identifier for this highlight.
    /// This should be a random UUID
    QString id;

    friend QDebug operator<<(QDebug dbg, const BadgeHighlight &v);

    void rebuildBadgeCheck();

    bool isValid() const
    {
        return !this->badgeName.isEmpty();
    }

    bool isMatch(const TwitchBadge &badge) const;
    bool compare(const QString &id, const TwitchBadge &badge) const;

    bool isMulti{false};
    bool hasVersions{false};
    QStringList multiBadges;
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::BadgeHighlight> {
    using H = chatterino::highlights::BadgeHighlight;

    static rapidjson::Value get(const H &value,
                                rapidjson::Document::AllocatorType &a)
    {
        using namespace chatterino;

        rapidjson::Value ret(rapidjson::kObjectType);

        rj::set(ret, "id", value.id, a);
        rj::set(ret, "type", H::TYPE, a);
        rj::setOptionally(ret, "name", value.name, a);
        rj::setOptionally(ret, "enabled", value.enabled, a);
        rj::setOptionally(ret, "badgeName", value.badgeName, a);
        rj::setOptionally(ret, "displayName", value.displayName, a);
        rj::setOptionally(ret, "showInMentions", value.showInMentions, a);
        rj::setOptionally(ret, "alert", value.alert, a);
        rj::setOptionally(ret, "playSound", value.playSound, a);

        if (!value.customSoundURL.isEmpty())
        {
            rj::set(ret, "customSoundURL", value.customSoundURL.toString(), a);
        }

        if (value.backgroundColor->isValid())
        {
            rj::set(ret, "backgroundColor",
                    value.backgroundColor->name(QColor::HexArgb), a);
        }

        return ret;
    }
};

template <>
struct Deserialize<chatterino::highlights::BadgeHighlight> {
    using H = chatterino::highlights::BadgeHighlight;

    static H get(const rapidjson::Value &value, bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return {u"invalid"};
        }

        if (!chatterino::highlights::matchesType(value, H::TYPE))
        {
            PAJLADA_REPORT_ERROR(error)
            return {u"invalid"};
        }

        QString id;
        if (!chatterino::rj::getSafe(value, "id", id))
        {
            PAJLADA_REPORT_ERROR(error)
            return {u"invalid"};
        }

        H h(id);

        chatterino::rj::getSafe(value, "name", h.name);
        chatterino::rj::getSafe(value, "enabled", h.enabled);
        chatterino::rj::getSafe(value, "badgeName", h.badgeName);
        chatterino::rj::getSafe(value, "displayName", h.displayName);
        chatterino::rj::getSafe(value, "showInMentions", h.showInMentions);
        chatterino::rj::getSafe(value, "alert", h.alert);
        chatterino::rj::getSafe(value, "playSound", h.playSound);

        QString tmpCustomSoundURL;
        chatterino::rj::getSafe(value, "customSoundURL", tmpCustomSoundURL);
        if (!tmpCustomSoundURL.isEmpty())
        {
            h.customSoundURL.setUrl(tmpCustomSoundURL);
        }

        QString tmpBackgroundColor;
        chatterino::rj::getSafe(value, "backgroundColor", tmpBackgroundColor);
        h.backgroundColor = std::make_shared<QColor>(tmpBackgroundColor);

        h.rebuildBadgeCheck();

        return h;
    }
};

}  // namespace pajlada

// QDebug operator<<(QDebug dbg, const chatterino::BadgeHighlight &v);
