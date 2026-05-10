// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/SharedHighlight2.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

#include <QIcon>
#include <QStringBuilder>

namespace chatterino::highlights {

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
    return this->outcome.showInMentions.value_or(true);
}

void SharedHighlight2::setShowInMentions(std::optional<bool> newValue)
{
    this->outcome.showInMentions = newValue;
}

bool SharedHighlight2::shouldHighlightTaskbar() const
{
    return this->outcome.alert.value_or(true);
}

void SharedHighlight2::setHighlightTaskbar(std::optional<bool> newValue)
{
    this->outcome.alert = newValue;
}

bool SharedHighlight2::isRegex() const
{
    return this->regex.value_or(false);
}

void SharedHighlight2::setRegex(std::optional<bool> newValue)
{
    this->regex = newValue;
}

bool SharedHighlight2::isCaseSensitive() const
{
    return this->caseSensitive.value_or(false);
}

void SharedHighlight2::setCaseSensitive(std::optional<bool> newValue)
{
    this->caseSensitive = newValue;
}

bool SharedHighlight2::shouldPlaySound() const
{
    return this->outcome.playSound.value_or(false);
}

void SharedHighlight2::setPlaySound(std::optional<bool> newValue)
{
    qInfo() << "XXX:" << this->pattern << "setPlaySound" << newValue;
    this->outcome.playSound = newValue;
}

QUrl SharedHighlight2::getSoundUrl() const
{
    return this->outcome.customSoundURL;
}

void SharedHighlight2::setSoundUrl(const QUrl &newValue)
{
    this->outcome.customSoundURL = newValue;
}

std::shared_ptr<QColor> SharedHighlight2::getBackgroundColor() const
{
    return this->outcome.backgroundColor;
}

void SharedHighlight2::setBackgroundColor(const QColor &newValue)
{
    this->outcome.backgroundColor = std::make_shared<QColor>(newValue);
}

QIcon SharedHighlight2::getType() const
{
    return QIcon{":/buttons/settings-darkMode.svg"};
}

bool SharedHighlight2::willPlayAnySound() const
{
    return this->outcome.playSound.value_or(false);
}

bool SharedHighlight2::willPlayCustomSound() const
{
    return this->willPlayAnySound() && !this->outcome.customSoundURL.isEmpty();
}

QDebug operator<<(QDebug dbg, const SharedHighlight2 &v)
{
    const auto &backgroundColorPtr = v.getBackgroundColor();
    QColor backgroundColor;
    if (backgroundColorPtr)
    {
        backgroundColor = *backgroundColorPtr;
    }
    dbg.nospace() << "SharedHighlight2("
                  << "name:" << v.name << ',' << "pattern:" << v.pattern << ','
                  << "enabled:" << v.isEnabled() << ','
                  << "showInMentions:" << v.shouldShowInMentions() << ','
                  << "alert:" << v.shouldHighlightTaskbar() << ','
                  << "playSound:" << v.shouldPlaySound() << ','
                  << "isRegex:" << v.isRegex() << ','
                  << "isCaseSensitive:" << v.isCaseSensitive() << ','
                  << "customSoundURL:" << v.getSoundUrl() << ','
                  << "backgroundColor:" << backgroundColor << ')';

    return dbg;
}

}  // namespace chatterino::highlights
