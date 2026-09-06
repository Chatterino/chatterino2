// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/FilterHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

#include <QIcon>
#include <QStringBuilder>

namespace chatterino::highlights {

FilterHighlight::FilterHighlight(QStringView _id)
    : id(_id)
{
    this->rebuildFilter();
}

HighlightCheck FilterHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;
    using Params = HighlightCheck::Params;

    if (!this->filter)
    {
        return {};
    }

    return {
        [highlight = *this](const Params &p) -> std::optional<HighlightResult> {
            assert(highlight.filter);

            auto res = highlight.filter->execute(p.runContext);
            if (!res.toBool())
            {
                return std::nullopt;
            }

            return HighlightResult{
                .ids = {highlight.getID().toString()},
                .alert = highlight.outcome.alert.value_or(H::ALERT_DEFAULT),
                .sound = highlight.outcome.soundURL,
                .color = highlight.outcome.getBackgroundColor(),
                .showInMentions = highlight.outcome.showInMentions.value_or(
                    H::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

QString FilterHighlight::getError() const
{
    return this->error;
}

void FilterHighlight::rebuildFilter()
{
    auto res = filters::Filter::fromString(this->filterText);
    if (std::holds_alternative<filters::Filter>(res))
    {
        this->filter = std::make_shared<filters::Filter>(
            std::move(std::get<filters::Filter>(res)));
        this->error.clear();
    }
    else
    {
        this->error = std::move(std::get<filters::FilterError>(res)).message;
        this->filter.reset();
    }
}

QDebug operator<<(QDebug dbg, const FilterHighlight &v)
{
    dbg.nospace() << "FilterHighlight("
                  << "name:" << v.name << ',' << "pattern:" << v.filterText
                  << ',' << "enabled:" << v.enabled << ','
                  << "sound:" << v.outcome.soundURL << ')';

    return dbg;
}

}  // namespace chatterino::highlights
