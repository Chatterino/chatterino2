#pragma once

#include <QString>

namespace chatterino {
namespace providers {
namespace twitch {

bool TrimChannelName(const QString &channelName, QString &outChannelName);

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
