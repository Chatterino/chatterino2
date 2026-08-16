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
    using H = std::remove_pointer_t<decltype(this)>;
    using Params = HighlightCheck::Params;

    if (!this->isValid())
    {
        return {};
    }

    return {
        [highlight = *this](const Params &p) -> std::optional<HighlightResult> {
            if (p.self)
            {
                // Phrase checks should ignore highlights from the user
                return std::nullopt;
            }

            if (!highlight.isMatch(p.originalMessage))
            {
                qCDebug(LOG)
                    << "NO MATCH - compared with highlight" << highlight;
                return std::nullopt;
            }

            qCDebug(LOG) << "MATCH - compared with highlight" << highlight
                         << highlight.outcome.sound
                         << highlight.outcome.soundURL;

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
                  << "sound:" << v.outcome.soundURL << ','
                  << "isRegex:" << v.regex << ','
                  << "isCaseSensitive:" << v.caseSensitive << ','
                  << "outcome:" << v.outcome << ')';

    return dbg;
}

}  // namespace chatterino::highlights
