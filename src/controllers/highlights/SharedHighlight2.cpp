// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/SharedHighlight2.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "singletons/Resources.hpp"

#include <QStringBuilder>

namespace chatterino {

namespace {

constexpr QStringView REGEX_START_BOUNDARY(u"(?:\\b|\\s|^)");
constexpr QStringView REGEX_END_BOUNDARY(u"(?:\\b|\\s|$)");

}  // namespace

bool SharedHighlight2::isEnabled() const
{
    return this->enabled.value_or(true);
}

void SharedHighlight2::setEnabled(std::optional<bool> newValue)
{
    this->enabled = newValue;
}

bool SharedHighlight2::shouldShowInMentions() const
{
    return this->showInMentions.value_or(true);
}

void SharedHighlight2::setShowInMentions(std::optional<bool> newValue)
{
    this->showInMentions = newValue;
}

bool SharedHighlight2::shouldHighlightTaskbar() const
{
    return this->alert.value_or(true);
}

void SharedHighlight2::setHighlightTaskbar(std::optional<bool> newValue)
{
    this->alert = newValue;
}

bool SharedHighlight2::shouldPlaySound() const
{
    return this->alert.value_or(false);
}

void SharedHighlight2::setPlaySound(std::optional<bool> newValue)
{
    this->alert = newValue;
}

QUrl SharedHighlight2::getSoundUrl() const
{
    return this->customSoundURL;
}

void SharedHighlight2::setSoundUrl(const QUrl &newValue)
{
    this->customSoundURL = newValue;
}

QPixmap SharedHighlight2::getType() const
{
    if (this->pattern == "my phrase")
    {
        return getResources().buttons.settings_darkMode.scaled(24, 24);
    }

    if (this->pattern == "user")
    {
        return getResources().buttons.account_darkMode.scaled(24, 24);
    }

    if (this->pattern == "badge")
    {
        return getResources().buttons.vip.scaled(24, 24);
    }

    return getResources().buttons.text;
}

bool SharedHighlight2::willPlayAnySound() const
{
    return this->playSound.value_or(false);
}

bool SharedHighlight2::willPlayCustomSound() const
{
    return this->willPlayAnySound() && !this->customSoundURL.isEmpty();
}

HighlightCheck SharedHighlight2::buildCheck() const
{
    return {
        [highlight = *this](const auto &args, const auto &badges,
                            const auto &senderName, const auto &originalMessage,
                            const auto &flags,
                            const auto self) -> std::optional<HighlightResult> {
            (void)args;        // unused
            (void)senderName;  // unused
            (void)flags;       // unused
            (void)self;        // unused
            (void)badges;      // unused

            if (originalMessage == highlight.pattern)
            {
                // return HighlightResult{
                //     .alert = highlight.alert,
                //     .playSound = highlight.playSound,
                //     .customSoundUrl = highlight.customSoundURL,
                //     .color = highlight.backgroundColor,
                //     .showInMentions = highlight.showInMentions,
                // };
                return HighlightResult{
                    highlight.alert.value_or(false),
                    highlight.playSound.value_or(false),
                    highlight.customSoundURL,
                    highlight.backgroundColor,
                    highlight.showInMentions.value_or(false),
                };
            }

            return std::nullopt;
        },
    };
}

// TODO: reimplement?
// bool SharedHighlight2::isMatch(const QString &subject) const
// {
//     return this->isValid() && this->regex_.match(subject).hasMatch();
// }

}  // namespace chatterino

QDebug operator<<(QDebug dbg, const chatterino::SharedHighlight2 &v)
{
    dbg.nospace() << "SharedHighlight2("
                  << "name:" << v.name << ',' << "pattern:" << v.pattern << ','
                  << "enabled:" << v.isEnabled() << ','
                  << "showInMentions:" << v.shouldShowInMentions() << ','
                  << "alert:" << v.shouldHighlightTaskbar() << ','
                  << "playSound:" << v.shouldPlaySound() << ','
                  << "isRegex:" << v.isRegex << ','
                  << "isCaseSensitive:" << v.isCaseSensitive << ','
                  << "customSoundURL:" << v.getSoundUrl() << ')';

    return dbg;
}
