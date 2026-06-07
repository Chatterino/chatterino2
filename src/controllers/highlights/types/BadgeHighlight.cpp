// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/BadgeHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "util/IrcHelpers.hpp"

#include <QStringBuilder>

namespace chatterino::highlights {

BadgeHighlight::BadgeHighlight(QStringView _id)
    : id(_id)
{
    this->rebuildBadgeCheck();
}

HighlightCheck BadgeHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;

    return {
        [highlight = *this](
            const auto &args, const auto &twitchBadges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)args;             // unused
            (void)senderName;       // unused
            (void)originalMessage;  // unused
            (void)flags;            // unused
            (void)self;             // unused
            (void)runContext;       // unused

            for (const TwitchBadge &badge : twitchBadges)
            {
                if (highlight.isMatch(badge))
                {
                    return HighlightResult{
                        .ids = {highlight.getID().toString()},
                        .alert =
                            highlight.outcome.alert.value_or(H::ALERT_DEFAULT),
                        .playSound = highlight.outcome.playSound.value_or(
                            H::PLAY_SOUND_DEFAULT),
                        .customSoundUrl = highlight.outcome.customSoundURL,
                        .color = highlight.outcome.getBackgroundColor(),
                        .showInMentions =
                            highlight.outcome.showInMentions.value_or(
                                H::SHOW_IN_MENTIONS_DEFAULT),
                    };
                }
            }

            return std::nullopt;
        },
    };
}

void BadgeHighlight::rebuildBadgeCheck()
{
    // check badgeName at initialization to reduce cost per isMatch call
    this->hasVersions = this->badgeName.contains("/");
    this->isMulti = this->badgeName.contains(",");
    if (this->isMulti)
    {
        this->multiBadges = this->badgeName.split(",");
    }
}

bool BadgeHighlight::isMatch(const TwitchBadge &badge) const
{
    if (!this->isMulti)
    {
        return this->compare(this->badgeName, badge);
    }

    return std::ranges::any_of(this->multiBadges,
                               [badge, this](const auto &id) {
                                   return this->compare(id, badge);
                               });
}

bool BadgeHighlight::compare(const QString &id, const TwitchBadge &badge) const
{
    if (this->hasVersions)
    {
        auto parts = slashKeyValue(id);
        return parts.first.compare(badge.key_, Qt::CaseInsensitive) == 0 &&
               parts.second.compare(badge.value_, Qt::CaseInsensitive) == 0;
    }

    return id.compare(badge.key_, Qt::CaseInsensitive) == 0;
}

QDebug operator<<(QDebug dbg, const BadgeHighlight &v)
{
    dbg.nospace() << "BadgeHighlight("
                  << "name:" << v.name << ',' << "badgeName:" << v.badgeName
                  << ',' << "displayName:" << v.displayName << ','
                  << "enabled:" << v.enabled << ','
                  << "playSound:" << v.outcome.playSound << ')';

    return dbg;
}

}  // namespace chatterino::highlights
