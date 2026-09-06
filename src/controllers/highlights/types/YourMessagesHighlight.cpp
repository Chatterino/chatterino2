// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/YourMessagesHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

namespace chatterino::highlights {

HighlightCheck YourMessagesHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;
    using Params = HighlightCheck::Params;

    return {
        [highlight = *this](const Params &p) -> std::optional<HighlightResult> {
            if (!p.self)
            {
                return std::nullopt;
            }

            // User has defined a color: std::shared_ptr<QColor>("#ff00ff")
            // User has not defined a color, should use default: std::shared_ptr<QColor> = {}; // unset shared ptr
            // User wants NO color, should fall through: std::shared_ptr<QColor> = std::shared_ptr<QColor>({}) // invalid QColor

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

QDebug operator<<(QDebug dbg, const YourMessagesHighlight &v)
{
    dbg.nospace() << "YourMessagesHighlight("        //
                  << "name:" << v.name               //
                  << ',' << "enabled:" << v.enabled  //
                  << ',' << "outcome:" << v.outcome  //
                  << ')';

    return dbg;
}

}  // namespace chatterino::highlights
