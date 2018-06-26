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

    providers::twitch::TwitchEmotes twitch;
    providers::bttv::BTTVEmotes bttv;
    providers::ffz::FFZEmotes ffz;
    providers::emoji::Emojis emojis;

    GIFTimer gifTimer;
};

}  // namespace chatterino
