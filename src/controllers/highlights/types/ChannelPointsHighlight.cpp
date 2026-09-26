// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/ChannelPointsHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "messages/MessageFlag.hpp"

namespace chatterino::highlights {

HighlightCheck ChannelPointsHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;
    using Params = HighlightCheck::Params;

    return {
        [highlight = *this](const Params &p) -> std::optional<HighlightResult> {
            if (p.messageFlags.hasAny({
                    MessageFlag::RedeemedHighlight,
                    MessageFlag::RedeemedChannelPointReward,
                }))
            {
                return highlight.outcome.makeSimpleResult<H>();
            }

            return std::nullopt;
        },
    };
}

QDebug operator<<(QDebug dbg, const ChannelPointsHighlight &v)
{
    dbg.nospace() << "ChannelPointsHighlight("       //
                  << "name:" << v.name               //
                  << ',' << "enabled:" << v.enabled  //
                  << ',' << "outcome:" << v.outcome  //
                  << ')';

    return dbg;
}

}  // namespace chatterino::highlights
