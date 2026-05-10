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

bool UserHighlight::isEnabled() const
{
    return this->enabled.value_or(true);
}

bool UserHighlight::shouldShowInMentions() const
{
    return this->showInMentions.value_or(true);
}

void UserHighlight::setShowInMentions(std::optional<bool> newValue)
{
    this->showInMentions = newValue;
}

bool UserHighlight::shouldHighlightTaskbar() const
{
    return this->alert.value_or(true);
}

void UserHighlight::setHighlightTaskbar(std::optional<bool> newValue)
{
    this->alert = newValue;
}

bool UserHighlight::shouldPlaySound() const
{
    return this->playSound.value_or(false);
}

void UserHighlight::setPlaySound(std::optional<bool> newValue)
{
    qInfo() << "XXX:" << this->username << "setPlaySound" << newValue;
    this->playSound = newValue;
}

QUrl UserHighlight::getSoundUrl() const
{
    return this->customSoundURL;
}

void UserHighlight::setSoundUrl(const QUrl &newValue)
{
    this->customSoundURL = newValue;
}

std::shared_ptr<QColor> UserHighlight::getBackgroundColor() const
{
    return this->backgroundColor;
}

void UserHighlight::setBackgroundColor(const QColor &newValue)
{
    this->backgroundColor = std::make_shared<QColor>(newValue);
}

QIcon UserHighlight::getType() const
{
    return QIcon{":/settings/accounts.svg"};
}

bool UserHighlight::willPlayAnySound() const
{
    return this->playSound.value_or(false);
}

bool UserHighlight::willPlayCustomSound() const
{
    return this->willPlayAnySound() && !this->customSoundURL.isEmpty();
}

HighlightCheck UserHighlight::buildCheck() const
{
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
                highlight.shouldHighlightTaskbar(), highlight.shouldPlaySound(),
                highlight.customSoundURL,           highlight.backgroundColor,
                highlight.shouldShowInMentions(),
            };
        },
    };
}

QDebug operator<<(QDebug dbg, const UserHighlight &v)
{
    const auto &backgroundColorPtr = v.getBackgroundColor();
    QColor backgroundColor;
    if (backgroundColorPtr)
    {
        backgroundColor = *backgroundColorPtr;
    }
    dbg.nospace() << "UserHighlight("
                  << "name:" << v.name << ',' << "username:" << v.username
                  << ',' << "enabled:" << v.enabled << ','
                  << "showInMentions:" << v.shouldShowInMentions() << ','
                  << "alert:" << v.shouldHighlightTaskbar() << ','
                  << "playSound:" << v.playSound << ','
                  << "customSoundURL:" << v.getSoundUrl() << ','
                  << "backgroundColor:" << backgroundColor << ')';

    return dbg;
}

}  // namespace chatterino::highlights
