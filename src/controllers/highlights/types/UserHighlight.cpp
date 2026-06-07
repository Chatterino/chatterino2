// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/UserHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

#include <qicon.h>
#include <QStringBuilder>

namespace chatterino::highlights {

UserHighlight::UserHighlight(QStringView _id)
    : id(_id)
{
}

HighlightCheck UserHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;

    return {
        [highlight = *this](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)args;             // unused
            (void)originalMessage;  // unused
            (void)flags;            // unused
            (void)self;             // unused
            (void)badges;           // unused
            (void)runContext;       // unused

            if (highlight.username.compare(senderName, Qt::CaseInsensitive) !=
                0)
            {
                return std::nullopt;
            }

            return HighlightResult{
                .ids = {highlight.getID().toString()},
                .alert = highlight.outcome.alert.value_or(H::ALERT_DEFAULT),
                .playSound =
                    highlight.outcome.playSound.value_or(H::PLAY_SOUND_DEFAULT),
                .customSoundUrl = highlight.outcome.customSoundURL,
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
                  << "playSound:" << v.outcome.playSound << ')';

    return dbg;
}

}  // namespace chatterino::highlights
