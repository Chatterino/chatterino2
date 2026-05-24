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
    return {
        [highlight = *this](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)args;             // unused
            (void)badges;           // unused
            (void)senderName;       // unused
            (void)originalMessage;  // unused
            (void)flags;            // unused
            (void)self;             // unused

            assert(highlight.filter);

            auto res = highlight.filter->execute(runContext);
            if (!res.toBool())
            {
                return std::nullopt;
            }

            return HighlightResult{
                highlight.outcome.alert.value_or(
                    FilterHighlight::ALERT_DEFAULT),
                highlight.outcome.playSound.value_or(
                    FilterHighlight::PLAY_SOUND_DEFAULT),
                highlight.outcome.customSoundURL,
                highlight.outcome.backgroundColor,
                highlight.outcome.showInMentions.value_or(
                    FilterHighlight::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

void FilterHighlight::rebuildFilter()
{
    auto res = filters::Filter::fromString(this->filterText);
    if (std::holds_alternative<filters::Filter>(res))
    {
        this->filter = std::make_shared<filters::Filter>(
            std::move(std::get<filters::Filter>(res)));
    }
    else
    {
        this->filter.reset();
        // TODO: Forward error
    }
}

QDebug operator<<(QDebug dbg, const FilterHighlight &v)
{
    dbg.nospace() << "FilterHighlight("
                  << "name:" << v.name << ',' << "pattern:" << v.filterText
                  << ',' << "enabled:" << v.enabled << ','
                  << "playSound:" << v.outcome.playSound << ')';

    return dbg;
}

}  // namespace chatterino::highlights
