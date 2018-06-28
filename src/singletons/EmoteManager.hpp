#pragma once

#define GIF_FRAME_LENGTH 33

#include "providers/bttv/BttvEmotes.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "singletons/helper/GifTimer.hpp"

namespace chatterino {

class EmoteManager
{
public:
    ~EmoteManager() = delete;

    void initialize();

    bool isIgnoredEmote(const QString &emote);

    TwitchEmotes twitch;
    BTTVEmotes bttv;
    FFZEmotes ffz;
    Emojis emojis;

    GIFTimer gifTimer;
};

}  // namespace chatterino
