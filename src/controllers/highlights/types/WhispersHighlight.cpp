// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/WhispersHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "messages/MessageParseArgs.hpp"

namespace chatterino::highlights {

HighlightCheck WhispersHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;
    using Params = HighlightCheck::Params;

    return {
        [highlight = *this](const Params &p) -> std::optional<HighlightResult> {
            if (!p.args.isReceivedWhisper)
            {
                return std::nullopt;
            }

            std::shared_ptr<QColor> backgroundColor;

            return HighlightResult{
                .ids = {H::ID.toString()},
                .alert = highlight.outcome.alert.value_or(H::ALERT_DEFAULT),
                .sound = highlight.outcome.soundURL,
                .color = highlight.outcome.getBackgroundColor(),
                .showInMentions = highlight.outcome.showInMentions.value_or(
                    H::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

QDebug operator<<(QDebug dbg, const WhispersHighlight &v)
{
    dbg.nospace() << "WhispersHighlight("            //
                  << "name:" << v.name               //
                  << ',' << "enabled:" << v.enabled  //
                  << ',' << "outcome:" << v.outcome  //
                  << ')';

    return dbg;
}

}  // namespace chatterino::highlights
