#pragma once

#include <QString>

namespace chatterino {

QString const TWITCH_USERNAME_PATTERN("^[a-zA-Z0-9][a-zA-Z0-9_]{0,24}$");

void openTwitchUsercard(const QString channel, const QString username);

// stripUserName removes any @ prefix or , suffix to make it more suitable for command use
void stripUserName(QString &userName);

// stripChannelName removes any @ prefix or , suffix to make it more suitable for command use
void stripChannelName(QString &channelName);

}  // namespace chatterino
