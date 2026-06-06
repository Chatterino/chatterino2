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
                .ids = {highlight.getID().toString()},
                .alert = highlight.outcome.alert.value_or(H::ALERT_DEFAULT),
                .playSound =
                    highlight.outcome.playSound.value_or(H::PLAY_SOUND_DEFAULT),
                .customSoundUrl = highlight.outcome.customSoundURL,
                .color = highlight.outcome.backgroundColor,
                .showInMentions = highlight.outcome.showInMentions.value_or(
                    H::SHOW_IN_MENTIONS_DEFAULT),
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
