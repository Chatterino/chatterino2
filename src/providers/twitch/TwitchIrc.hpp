#pragma once

#include "messages/Emote.hpp"
#include "providers/twitch/TwitchBadge.hpp"

#include <QString>
#include <QVariantMap>

#include <unordered_map>

namespace chatterino {

struct TwitchEmoteOccurrence {
    int start;
    int end;
    EmotePtr ptr;
    EmoteName name;

    bool operator==(const TwitchEmoteOccurrence &other) const
    {
        return std::tie(this->start, this->end, this->ptr, this->name) ==
               std::tie(other.start, other.end, other.ptr, other.name);
    }
};

/// @brief Parses the `badge-info` tag of an IRC message
///
/// The `badge-info` tag maps badge-names to a value. Subscriber badges, for
/// example, are mapped to the number of months the chatter is subscribed for.
///
/// **Example**:
/// `badge-info=subscriber/22` would be parsed as `{ subscriber => 22 }`
///
/// @param tags The tags of the IRC message
/// @returns A map of badge-names to their values
std::unordered_map<QString, QString> parseBadgeInfoTag(const QVariantMap &tags);

/// @brief Parses the `badges` tag of an IRC message
///
/// The `badges` tag contains a comma separated list of key-value elements which
/// make up the name and version of each badge.
///
/// **Example**:
/// `badges=broadcaster/1,subscriber/18` would be parsed as
/// `[(broadcaster, 1), (subscriber, 18)]`
///
/// @param tags The tags of the IRC message
/// @returns A list of badges (name and version)
std::vector<Badge> parseBadgeTag(const QVariantMap &tags);

/// @brief Parses Twitch emotes in an IRC message
///
/// @param tags The tags of the IRC message
/// @param content The message text. This might be shortened due to skipping
///                content at the start. `messageOffset` describes this offset.
/// @param messageOffset The offset of `content` compared to the original
///                      message text. Used for calculating indices into the
///                      message. An offset of 3, for example, indicates that
///                      `content` excludes the first three characters of the
///                      original message (`@a foo` (original message) -> `foo`
///                      (content)).
/// @returns A list of emotes and their positions
std::vector<TwitchEmoteOccurrence> parseTwitchEmotes(const QVariantMap &tags,
                                                     const QString &content,
                                                     int messageOffset);

}  // namespace chatterino
