#pragma once

#include <QString>

namespace chatterino {

enum class ShowIgnoredUsersMessages { Never, IfModerator, IfBroadcaster };

struct IgnoredMessageParameters {
    QString message;

    QString twitchUserID;
    bool isMod;
    bool isBroadcaster;
};

bool isIgnoredMessage(IgnoredMessageParameters &&params);

}  // namespace chatterino
