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

void appendTwitchEmoteOccurrences(QStringView emote,
                                  std::vector<TwitchEmoteOccurrence> &out,
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
        auto [fromStr, toStr] = splitOnce(occurrence, u'-');
        bool fromOk = false;
        bool toOk = false;
        uint16_t from = fromStr.toUShort(&fromOk);
        uint16_t to = toStr.toUShort(&toOk);
        if (!fromOk || !toOk)
        {
            qCDebug(chatterinoTwitch) << "Invalid emote range:" << occurrence;
            continue;
        }
        if (from > to || std::cmp_less(from, messageOffset))
        {
            qCDebug(chatterinoTwitch)
                << "Out of bounds emote range:" << occurrence
                << "offset:" << messageOffset;
            continue;
        }
        to -= messageOffset;
        from -= messageOffset;
        if (to >= codepointToUtf16Idx.size())
        {
            qCDebug(chatterinoTwitch)
                << "Out of bounds emote range:" << occurrence
                << "max-codepoints:" << codepointToUtf16Idx.size();
            return;
        }

        auto start = codepointToUtf16Idx[from];
        auto end = codepointToUtf16Idx[to];
        assert(start <= end && end < originalMessage.length() &&
               "Bad codepointToUtf16Idx list");

        auto name = EmoteName{
            originalMessage.sliced(start, end - start + 1).toString()};
        auto ptr =
            app->getEmotes()->getTwitchEmotes()->getOrCreateEmote(id, name);
        if (!ptr)
        {
            qCDebug(chatterinoTwitch) << "Invalid emote:" << id.string;
            continue;
        }

        out.emplace_back(TwitchEmoteOccurrence{
            .start = start,
            .end = end,
            .ptr = ptr,
            .name = name,
        });
    }
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

std::vector<TwitchEmoteOccurrence> parseTwitchEmotes(Communi::TagsRef tags,
                                                     QStringView content,
                                                     int messageOffset)
{
    // Twitch emotes
    std::vector<TwitchEmoteOccurrence> twitchEmotes;

    auto emotesTag = tags.getOrEmpty("emotes");

    if (emotesTag.isEmpty() ||
        content.size() > std::numeric_limits<uint16_t>::max())
    {
        return twitchEmotes;
    }

    QVarLengthArray<uint16_t, 128> codepointToUtf16Idx;
    // We know the maximum length for the message, because
    // `#code-points <= #utf16-code-units` is always true.
    codepointToUtf16Idx.reserve(content.size());
    for (qsizetype i = 0; i < content.size(); ++i)
    {
        if (!content.at(i).isLowSurrogate())
        {
            codepointToUtf16Idx.push_back(i);
        }
    }
    for (const auto emote : emotesTag.tokenize(u'/'))
    {
        appendTwitchEmoteOccurrences(emote, twitchEmotes, codepointToUtf16Idx,
                                     content, messageOffset);
    }

    return twitchEmotes;
}

}  // namespace chatterino
