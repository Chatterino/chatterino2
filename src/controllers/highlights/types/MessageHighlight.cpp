// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/MessageHighlight.hpp"

#include "common/QLogging.hpp"
#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

#include <QIcon>
#include <QStringBuilder>

namespace chatterino::highlights {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoHighlights;

// TODO: should this be shared somewhere
constexpr QStringView REGEX_START_BOUNDARY(u"(?:\\b|\\s|^)");
constexpr QStringView REGEX_END_BOUNDARY(u"(?:\\b|\\s|$)");

}  // namespace

MessageHighlight::MessageHighlight(QStringView _id)
    : id(_id)
{
    this->rebuildInternalRegularExpression();
}

bool MessageHighlight::isEnabled() const
{
    return this->enabled.value_or(true);
}

void MessageHighlight::setEnabled(std::optional<bool> newValue)
{
    this->enabled = newValue;
}

bool MessageHighlight::shouldShowInMentions() const
{
    return this->outcome.showInMentions.value_or(true);
}

void MessageHighlight::setShowInMentions(std::optional<bool> newValue)
{
    this->outcome.showInMentions = newValue;
}

bool MessageHighlight::shouldHighlightTaskbar() const
{
    return this->outcome.alert.value_or(true);
}

void MessageHighlight::setHighlightTaskbar(std::optional<bool> newValue)
{
    this->outcome.alert = newValue;
}

bool MessageHighlight::isRegex() const
{
    return this->regex.value_or(false);
}

void MessageHighlight::setRegex(std::optional<bool> newValue)
{
    this->regex = newValue;
}

bool MessageHighlight::isCaseSensitive() const
{
    return this->caseSensitive.value_or(false);
}

void MessageHighlight::setCaseSensitive(std::optional<bool> newValue)
{
    this->caseSensitive = newValue;
    this->rebuildInternalRegularExpression();
}

bool MessageHighlight::shouldPlaySound() const
{
    return this->outcome.playSound.value_or(false);
}

void MessageHighlight::setPlaySound(std::optional<bool> newValue)
{
    this->outcome.playSound = newValue;
}

QUrl MessageHighlight::getSoundUrl() const
{
    return this->outcome.customSoundURL;
}

void MessageHighlight::setSoundUrl(const QUrl &newValue)
{
    this->outcome.customSoundURL = newValue;
}

std::shared_ptr<QColor> MessageHighlight::getBackgroundColor() const
{
    return this->outcome.backgroundColor;
}

void MessageHighlight::setBackgroundColor(const QColor &newValue)
{
    this->outcome.backgroundColor = std::make_shared<QColor>(newValue);
}

QIcon MessageHighlight::getType() const
{
    return QIcon{":/buttons/text.svg"};
}

bool MessageHighlight::willPlayAnySound() const
{
    return this->outcome.playSound.value_or(false);
}

bool MessageHighlight::willPlayCustomSound() const
{
    return this->willPlayAnySound() && !this->outcome.customSoundURL.isEmpty();
}

HighlightCheck MessageHighlight::buildCheck() const
{
    qCDebug(LOG) << "Rebuilding check" << this;

    return {
        [highlight = *this](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)args;        // unused
            (void)senderName;  // unused
            (void)flags;       // unused
            (void)self;        // unused
            (void)badges;      // unused
            (void)runContext;  // unused

            qCDebug(LOG) << "Comparing with highlight" << highlight;

            if (!highlight.isMatch(originalMessage))
            {
                return std::nullopt;
            }

            return HighlightResult{
                highlight.shouldHighlightTaskbar(),
                highlight.shouldPlaySound(),
                highlight.outcome.customSoundURL,
                highlight.outcome.backgroundColor,
                highlight.shouldShowInMentions(),
            };
        },
    };
}

void MessageHighlight::rebuildInternalRegularExpression()
{
    // TODO: this is inflexible
    if (this->isRegex())
    {
        this->regexPattern.setPattern(this->getPattern());
    }
    else
    {
        this->regexPattern.setPattern(
            REGEX_START_BOUNDARY %
            QRegularExpression::escape(this->getPattern()) %
            REGEX_END_BOUNDARY);
    }

    if (this->isCaseSensitive())
    {
        this->regexPattern.setPatternOptions(
            QRegularExpression::UseUnicodePropertiesOption);
    }
    else
    {
        this->regexPattern.setPatternOptions(
            QRegularExpression::UseUnicodePropertiesOption |
            QRegularExpression::CaseInsensitiveOption);
    }
}

QDebug operator<<(QDebug dbg, const MessageHighlight &v)
{
    const auto &backgroundColorPtr = v.getBackgroundColor();
    QColor backgroundColor;
    if (backgroundColorPtr)
    {
        backgroundColor = *backgroundColorPtr;
    }
    dbg.nospace() << "MessageHighlight("
                  << "name:" << v.name << ',' << "pattern:" << v.pattern << ','
                  << "patternRegex:" << v.regexPattern << ','
                  << "enabled:" << v.enabled << ','
                  << "showInMentions:" << v.shouldShowInMentions() << ','
                  << "alert:" << v.shouldHighlightTaskbar() << ','
                  << "playSound:" << v.outcome.playSound << ','
                  << "isRegex:" << v.regex << ','
                  << "isCaseSensitive:" << v.caseSensitive << ','
                  << "customSoundURL:" << v.getSoundUrl() << ','
                  << "backgroundColor:" << backgroundColor << ','
                  << "outcome:" << v.outcome << ')';

    return dbg;
}

}  // namespace chatterino::highlights
