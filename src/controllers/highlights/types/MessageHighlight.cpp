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

}  // namespace

MessageHighlight::MessageHighlight(QStringView _id)
    : id(_id)
{
    this->rebuildInternalRegularExpression();
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

HighlightCheck MessageHighlight::buildCheck() const
{
    if (!this->isValid())
    {
        return {};
    }

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
                highlight.outcome.alert.value_or(
                    MessageHighlight::ALERT_DEFAULT),
                highlight.outcome.playSound.value_or(
                    MessageHighlight::PLAY_SOUND_DEFAULT),
                highlight.outcome.customSoundURL,
                highlight.outcome.backgroundColor,
                highlight.outcome.showInMentions.value_or(
                    MessageHighlight::SHOW_IN_MENTIONS_DEFAULT),
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
    dbg.nospace() << "MessageHighlight("
                  << "name:" << v.name << ',' << "pattern:" << v.pattern << ','
                  << "patternRegex:" << v.regexPattern << ','
                  << "enabled:" << v.enabled << ','
                  << "playSound:" << v.outcome.playSound << ','
                  << "isRegex:" << v.regex << ','
                  << "isCaseSensitive:" << v.caseSensitive << ','
                  << "outcome:" << v.outcome << ')';

    return dbg;
}

}  // namespace chatterino::highlights
