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

bool BadgeHighlight::isEnabled() const
{
    return this->enabled.value_or(true);
}

bool BadgeHighlight::shouldShowInMentions() const
{
    return this->showInMentions.value_or(true);
}

void BadgeHighlight::setShowInMentions(std::optional<bool> newValue)
{
    this->showInMentions = newValue;
}

bool BadgeHighlight::shouldHighlightTaskbar() const
{
    return this->alert.value_or(true);
}

void BadgeHighlight::setHighlightTaskbar(std::optional<bool> newValue)
{
    this->alert = newValue;
}

bool BadgeHighlight::shouldPlaySound() const
{
    return this->playSound.value_or(false);
}

void BadgeHighlight::setPlaySound(std::optional<bool> newValue)
{
    this->playSound = newValue;
}

QUrl BadgeHighlight::getSoundUrl() const
{
    return this->customSoundURL;
}

void BadgeHighlight::setSoundUrl(const QUrl &newValue)
{
    this->customSoundURL = newValue;
}

std::shared_ptr<QColor> BadgeHighlight::getBackgroundColor() const
{
    return this->backgroundColor;
}

void BadgeHighlight::setBackgroundColor(const QColor &newValue)
{
    this->backgroundColor = std::make_shared<QColor>(newValue);
}

void BadgeHighlight::debug() const
{
    qInfo() << "XXX: DEBUG UDH" << *this;
}

bool BadgeHighlight::willPlayAnySound() const
{
    return this->playSound.value_or(false);
}

bool BadgeHighlight::willPlayCustomSound() const
{
    return this->willPlayAnySound() && !this->customSoundURL.isEmpty();
}

HighlightCheck BadgeHighlight::buildCheck() const
{
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
                        highlight.shouldHighlightTaskbar(),
                        highlight.shouldPlaySound(),
                        highlight.customSoundURL,
                        highlight.backgroundColor,
                        highlight.shouldShowInMentions(),
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
    const auto &backgroundColorPtr = v.getBackgroundColor();
    QColor backgroundColor;
    if (backgroundColorPtr)
    {
        backgroundColor = *backgroundColorPtr;
    }
    dbg.nospace() << "BadgeHighlight("
                  << "name:" << v.name << ',' << "badgeName:" << v.badgeName
                  << ',' << "displayName:" << v.displayName << ','
                  << "enabled:" << v.enabled << ','
                  << "showInMentions:" << v.shouldShowInMentions() << ','
                  << "alert:" << v.shouldHighlightTaskbar() << ','
                  << "playSound:" << v.playSound << ','
                  << "customSoundURL:" << v.getSoundUrl() << ','
                  << "backgroundColor:" << backgroundColor << ')';

    return dbg;
}

}  // namespace chatterino::highlights
