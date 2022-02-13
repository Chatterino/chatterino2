#pragma once

#include <QString>

namespace chatterino {

void openTwitchUsercard(const QString channel, const QString username);

// stripUserName removes any @ prefix or , suffix to make it more suitable for command use
void stripUserName(QString &userName);

// stripChannelName removes any @ prefix or , suffix to make it more suitable for command use
void stripChannelName(QString &channelName);

}  // namespace chatterino
