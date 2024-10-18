#pragma once

#include <QString>

#include <vector>

namespace chatterino {

class IgnorePhrase;
struct TwitchEmoteOccurrence;

enum class ShowIgnoredUsersMessages { Never, IfModerator, IfBroadcaster };

struct IgnoredMessageParameters {
    QString message;

    QString twitchUserID;
    bool isMod;
    bool isBroadcaster;
};

bool isIgnoredMessage(IgnoredMessageParameters &&params);

/// @brief Processes replacement ignore-phrases for a message
///
/// @param phrases A list of IgnorePhrases to process. Block phrases as well as
/// 	           invalid phrases are ignored.
/// @param content The message text. This gets altered by replacements.
/// @param twitchEmotes A list of emotes present in the message. Occurrences
///                     that have been removed from the message will also be
///                     removed in this list. Similarly, if new emotes are added
///                     from a replacement, this list gets updated as well.
void processIgnorePhrases(const std::vector<IgnorePhrase> &phrases,
                          QString &content,
                          std::vector<TwitchEmoteOccurrence> &twitchEmotes);

}  // namespace chatterino
