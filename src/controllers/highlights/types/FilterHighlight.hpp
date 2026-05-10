// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/filters/lang/Filter.hpp"
#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/types/Common.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize/common.hpp>
#include <qcolor.h>
#include <qdebug.h>
#include <qpixmap.h>
#include <qregularexpression.h>
#include <QStringView>
#include <qurl.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <memory>
#include <optional>

class QIcon;

namespace chatterino::highlights {

struct FilterHighlight {
    static constexpr QStringView TYPE = u"filter";

    FilterHighlight(QStringView _id);

    QString getDefaultName() const
    {
        return this->filterText;
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
        return this->id;
    }

    bool operator==(const FilterHighlight &other) const = default;

    /// Returns true if this highlight should be enabled.
    ///
    /// If unconfigured, returns true.
    bool isEnabled() const;

    void setFilterText(const QString &newFilter)
    {
        this->filterText = newFilter;
        this->rebuildFilter();
    }

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

protected:
    std::optional<bool> enabled;

    /// Contains the filter (e.g. `message.content contains "forsen"`)
    QString filterText;

    std::shared_ptr<filters::Filter> filter;

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
    QIcon getType() const;

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

    friend QDebug operator<<(QDebug dbg, const FilterHighlight &v);

    void rebuildFilter();
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::FilterHighlight> {
    using H = chatterino::highlights::FilterHighlight;

    static rapidjson::Value get(const H &value,
                                rapidjson::Document::AllocatorType &a)
    {
        using namespace chatterino;

        rapidjson::Value ret(rapidjson::kObjectType);

        rj::set(ret, "id", value.id, a);
        rj::set(ret, "type", H::TYPE, a);

        rj::setOptionally(ret, "name", value.name, a);
        rj::setOptionally(ret, "enabled", value.enabled, a);
        rj::setOptionally(ret, "filterText", value.filterText, a);

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
struct Deserialize<chatterino::highlights::FilterHighlight> {
    using H = chatterino::highlights::FilterHighlight;

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
        chatterino::rj::getSafe(value, "filterText", h.filterText);

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

        h.rebuildFilter();

        return h;
    }
};

}  // namespace pajlada
