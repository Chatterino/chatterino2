// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/twitch/TwitchIrc.hpp"

#include "Application.hpp"
#include "common/Aliases.hpp"
#include "common/QLogging.hpp"
#include "controllers/emotes/EmoteController.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "util/Helpers.hpp"
#include "util/IrcHelpers.hpp"

#include <span>

namespace {

using namespace chatterino;
using namespace Qt::Literals;

void createSpecialOccurrence(QStringView occurrence,
                             std::vector<TwitchSpecialOccurrence> &out,
                             std::span<const uint16_t> codepointToUtf16Idx,
                             QStringView originalMessage, int messageOffset,
                             auto &&factory)
{
    auto [fromStr, toStr] = splitOnce(occurrence, u'-');
    bool fromOk = false;
    bool toOk = false;
    uint16_t from = fromStr.toUShort(&fromOk);
    uint16_t to = toStr.toUShort(&toOk);
    if (!fromOk || !toOk)
    {
        qCDebug(chatterinoTwitch) << "Invalid range:" << occurrence;
        return;
    }
    if (from > to || std::cmp_less(from, messageOffset))
    {
        qCDebug(chatterinoTwitch) << "Out of bounds range:" << occurrence
                                  << "offset:" << messageOffset;
        return;
    }
    to -= messageOffset;
    from -= messageOffset;
    if (to >= codepointToUtf16Idx.size() - 1)
    {
        qCDebug(chatterinoTwitch)
            << "Out of bounds range:" << occurrence
            << "max-codepoints:" << codepointToUtf16Idx.size();
        return;
    }

    auto start = codepointToUtf16Idx[from];
    auto end = codepointToUtf16Idx[to + 1];
    assert(start <= end && end <= originalMessage.length() &&
           "Bad codepointToUtf16Idx list");

    auto created = factory(originalMessage.sliced(start, end - start));
    if (!created.has_value())
    {
        return;
    }

    out.emplace_back(TwitchSpecialOccurrence{
        .start = start,
        .length = end - start,
        .data = *std::move(created),
    });
}

void appendTwitchEmoteOccurrences(QStringView emote,
                                  std::vector<TwitchSpecialOccurrence> &out,
                                  std::span<const uint16_t> codepointToUtf16Idx,
                                  QStringView originalMessage,
                                  int messageOffset)
{
    auto *app = getApp();

    auto [idRef, ranges] = splitOnce(emote, u':');
    if (ranges.empty())
    {
        return;
    }
    // FIXME: Add an EmoteIdView.
    auto id = EmoteId{idRef.toString()};

    for (const auto occurrence : ranges.tokenize(u','))
    {
        createSpecialOccurrence(
            occurrence, out, codepointToUtf16Idx, originalMessage,
            messageOffset,
            [&](QStringView nameStr) -> std::optional<TwitchEmoteOccurrence> {
                auto name = EmoteName{nameStr.toString()};
                auto ptr =
                    app->getEmotes()->getTwitchEmotes()->getOrCreateEmote(id,
                                                                          name);
                if (!ptr)
                {
                    qCDebug(chatterinoTwitch) << "Invalid emote:" << id.string;
                    return std::nullopt;
                }
                return TwitchEmoteOccurrence{.ptr = ptr, .name = name};
            });
    }
}

void appendTwitchGifOccurrence(QStringView gif,
                               std::vector<TwitchSpecialOccurrence> &out,
                               std::span<const uint16_t> codepointToUtf16Idx,
                               QStringView originalMessage, int messageOffset)
{
    // A single entry looks like "<range>|<gifID>|<gifURL>".
    auto [range, rest] = splitOnce(gif, u'|');
    auto [id, link] = splitOnce(rest, u'|');
    if (link.empty())
    {
        qCWarning(chatterinoTwitch) << "Invalid gif:" << gif;
        return;
    }

    createSpecialOccurrence(
        range, out, codepointToUtf16Idx, originalMessage, messageOffset,
        [&](QStringView /* nameStr */) -> std::optional<TwitchGifOccurrence> {
            return TwitchGifOccurrence{
                .id = id.toString(),
            };
        });
}

}  // namespace

namespace chatterino {

std::unordered_map<QString, QString> parseBadgeInfoTag(Communi::TagsRef tags)
{
    std::unordered_map<QString, QString> infoMap;

    auto infoIt = tags.get("badge-info");
    if (!infoIt)
    {
        return infoMap;
    }

    auto info = infoIt->split(',', Qt::SkipEmptyParts);

    for (const QString &badge : info)
    {
        infoMap.emplace(slashKeyValue(badge));
    }

    return infoMap;
}

std::vector<TwitchBadge> parseBadgeTag(Communi::TagsRef tags,
                                       const QString &tagName)
{
    std::vector<TwitchBadge> b;

    auto badgesIt = tags.get(tagName);
    if (!badgesIt)
    {
        return b;
    }

    auto badges = badgesIt->split(',', Qt::SkipEmptyParts);

    for (const QString &badge : badges)
    {
        if (!badge.contains('/'))
        {
            continue;
        }

        auto pair = slashKeyValue(badge);
        b.emplace_back(TwitchBadge{pair.first, pair.second});
    }

    return b;
}

std::vector<TwitchSpecialOccurrence> parseTwitchOccurrences(
    Communi::TagsRef tags, QStringView content, int messageOffset)
{
    std::vector<TwitchSpecialOccurrence> occurrences;

    auto emotesTag = tags.getOrEmpty("emotes");
    auto gifsTag = tags.getOrEmpty("gifs");

    if ((gifsTag.isEmpty() && emotesTag.isEmpty()) ||
        content.size() > std::numeric_limits<uint16_t>::max())
    {
        return occurrences;
    }

    QVarLengthArray<uint16_t, 128> codepointToUtf16Idx;
    // We know the maximum length for the message, because
    // `#code-points <= #utf16-code-units` is always true.
    codepointToUtf16Idx.reserve(content.size() + 1);
    for (qsizetype i = 0; i < content.size(); ++i)
    {
        if (!content.at(i).isLowSurrogate())
        {
            codepointToUtf16Idx.push_back(i);
        }
    }
    codepointToUtf16Idx.push_back(content.size());

    if (!emotesTag.isEmpty())
    {
        for (const auto emote : emotesTag.tokenize(u'/'))
        {
            appendTwitchEmoteOccurrences(emote, occurrences,
                                         codepointToUtf16Idx, content,
                                         messageOffset);
        }
    }

    if (!gifsTag.isEmpty())
    {
        for (const auto gif : gifsTag.tokenize(u','))
        {
            appendTwitchGifOccurrence(gif, occurrences, codepointToUtf16Idx,
                                      content, messageOffset);
        }
    }

    return occurrences;
}

}  // namespace chatterino
