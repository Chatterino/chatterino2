// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/UserHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

namespace chatterino::highlights {

UserHighlight::UserHighlight(QStringView _id)
    : id(_id)
{
}

HighlightCheck UserHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;
    using Params = HighlightCheck::Params;

    return {
        [highlight = *this](const Params &p) -> std::optional<HighlightResult> {
            if (highlight.username.compare(p.senderName, Qt::CaseInsensitive) !=
                0)
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

QDebug operator<<(QDebug dbg, const UserHighlight &v)
{
    dbg.nospace() << "UserHighlight("
                  << "name:" << v.name << ',' << "username:" << v.username
                  << ',' << "enabled:" << v.enabled << ','
                  << "sound:" << v.outcome.soundURL << ')';

    return dbg;
}

}  // namespace chatterino::highlights
