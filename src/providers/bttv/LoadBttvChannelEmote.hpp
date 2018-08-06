#pragma once

#include <functional>

class QString;

namespace chatterino {

class EmoteMap;
constexpr const char *bttvChannelEmoteApiUrl =
    "https://api.betterttv.net/2/channels/";

void loadBttvChannelEmotes(const QString &channelName,
                           std::function<void(EmoteMap &&)> callback);

}  // namespace chatterino
