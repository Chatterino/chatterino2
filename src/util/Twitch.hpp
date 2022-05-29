#pragma once

#include <QRegularExpression>
#include <QString>

namespace chatterino {

void openTwitchUsercard(const QString channel, const QString username);

// stripUserName removes any @ prefix or , suffix to make it more suitable for command use
void stripUserName(QString &userName);

// stripChannelName removes any @ prefix or , suffix to make it more suitable for command use
void stripChannelName(QString &channelName);

// Matches a strict Twitch user login.
// May contain lowercase a-z, 0-9, and underscores
// Must contain between 1 and 25 characters
// Must not start with an underscore
QRegularExpression twitchUserLoginRegexp();

// Matches a loose Twitch user login name.
// May contain lowercase and uppercase a-z, 0-9, and underscores
// Must contain between 1 and 25 characters
// Must not start with an underscore
QRegularExpression twitchUserNameRegexp();

}  // namespace chatterino
