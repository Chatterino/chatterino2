// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/filters/lang/Filter.hpp"
#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/types/Common.hpp"
#include "controllers/highlights/types/Outcome.hpp"
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
    static constexpr QStringView ICON_RESOURCE = u":/settings/filters.svg";

    static constexpr bool ENABLED_BY_DEFAULT = true;
    static constexpr bool SHOW_IN_MENTIONS_DEFAULT = true;
    static constexpr bool ALERT_DEFAULT = true;
    static constexpr bool PLAY_SOUND_DEFAULT = false;
    // TODO
    static constexpr QColor BACKGROUND_COLOR_DEFAULT = QColor(127, 63, 73, 127);

    FilterHighlight(QStringView _id);

    QString getDefaultName() const
    {
        return this->filterText;
    }

    QStringView getID() const
    {
        return this->id;
    }

    bool operator==(const FilterHighlight &other) const = default;

    void setFilterText(const QString &newFilter)
    {
        this->filterText = newFilter;
        this->rebuildFilter();
    }

    /// The display name/pretty name of this highlight.
    /// If empty, we will try to auto-generate something that makes sense (e.g. "Text contains 'foo'")
    QString name;

    std::optional<bool> enabled;

    /// Contains the filter (e.g. `message.content contains "forsen"`)
    QString filterText;

    std::shared_ptr<filters::Filter> filter;

    Outcome outcome{BACKGROUND_COLOR_DEFAULT};

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

        value.outcome.serialize(ret, a);

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

        h.outcome.deserialize(value);

        h.rebuildFilter();

        return h;
    }
};

}  // namespace pajlada
