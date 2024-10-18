#include "providers/twitch/TwitchIrc.hpp"

#include "Application.hpp"
#include "common/Aliases.hpp"
#include "common/QLogging.hpp"
#include "singletons/Emotes.hpp"
#include "util/IrcHelpers.hpp"

namespace {

using namespace chatterino;

void appendTwitchEmoteOccurrences(const QString &emote,
                                  std::vector<TwitchEmoteOccurrence> &vec,
                                  const std::vector<int> &correctPositions,
                                  const QString &originalMessage,
                                  int messageOffset)
{
    auto *app = getApp();
    if (!emote.contains(':'))
    {
        return;
    }

    auto parameters = emote.split(':');

    if (parameters.length() < 2)
    {
        return;
    }

    auto id = EmoteId{parameters.at(0)};

    auto occurrences = parameters.at(1).split(',');

    for (const QString &occurrence : occurrences)
    {
        auto coords = occurrence.split('-');

        if (coords.length() < 2)
        {
            return;
        }

        auto from = coords.at(0).toUInt() - messageOffset;
        auto to = coords.at(1).toUInt() - messageOffset;
        auto maxPositions = correctPositions.size();
        if (from > to || to >= maxPositions)
        {
            // Emote coords are out of range
            qCDebug(chatterinoTwitch)
                << "Emote coords" << from << "-" << to << "are out of range ("
                << maxPositions << ")";
            return;
        }

        auto start = correctPositions[from];
        auto end = correctPositions[to];
        if (start > end || start < 0 || end > originalMessage.length())
        {
            // Emote coords are out of range from the modified character positions
            qCDebug(chatterinoTwitch) << "Emote coords" << from << "-" << to
                                      << "are out of range after offsets ("
                                      << originalMessage.length() << ")";
            return;
        }

        auto name = EmoteName{originalMessage.mid(start, end - start + 1)};
        TwitchEmoteOccurrence emoteOccurrence{
            start,
            end,
            app->getEmotes()->getTwitchEmotes()->getOrCreateEmote(id, name),
            name,
        };
        if (emoteOccurrence.ptr == nullptr)
        {
            qCDebug(chatterinoTwitch)
                << "nullptr" << emoteOccurrence.name.string;
        }
        vec.push_back(std::move(emoteOccurrence));
    }
}

}  // namespace

namespace chatterino {

std::unordered_map<QString, QString> parseBadgeInfoTag(const QVariantMap &tags)
{
    std::unordered_map<QString, QString> infoMap;

    auto infoIt = tags.constFind("badge-info");
    if (infoIt == tags.end())
    {
        return infoMap;
    }

    auto info = infoIt.value().toString().split(',', Qt::SkipEmptyParts);

    for (const QString &badge : info)
    {
        infoMap.emplace(slashKeyValue(badge));
    }

    return infoMap;
}

std::vector<Badge> parseBadgeTag(const QVariantMap &tags)
{
    std::vector<Badge> b;

    auto badgesIt = tags.constFind("badges");
    if (badgesIt == tags.end())
    {
        return b;
    }

    auto badges = badgesIt.value().toString().split(',', Qt::SkipEmptyParts);

    for (const QString &badge : badges)
    {
        if (!badge.contains('/'))
        {
            continue;
        }

        auto pair = slashKeyValue(badge);
        b.emplace_back(Badge{pair.first, pair.second});
    }

    return b;
}

std::vector<TwitchEmoteOccurrence> parseTwitchEmotes(const QVariantMap &tags,
                                                     const QString &content,
                                                     int messageOffset)
{
    // Twitch emotes
    std::vector<TwitchEmoteOccurrence> twitchEmotes;

    auto emotesTag = tags.find("emotes");

    if (emotesTag == tags.end())
    {
        return twitchEmotes;
    }

    QStringList emoteString = emotesTag.value().toString().split('/');
    std::vector<int> correctPositions;
    for (int i = 0; i < content.size(); ++i)
    {
        if (!content.at(i).isLowSurrogate())
        {
            correctPositions.push_back(i);
        }
    }
    for (const QString &emote : emoteString)
    {
        appendTwitchEmoteOccurrences(emote, twitchEmotes, correctPositions,
                                     content, messageOffset);
    }

    return twitchEmotes;
}

}  // namespace chatterino
