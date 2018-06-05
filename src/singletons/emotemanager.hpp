#pragma once

#define GIF_FRAME_LENGTH 33

#include "providers/bttv/bttvemotes.hpp"
#include "providers/emoji/emojis.hpp"
#include "providers/ffz/ffzemotes.hpp"
#include "providers/twitch/twitchemotes.hpp"
#include "singletons/helper/giftimer.hpp"

#include <QString>

namespace chatterino {
namespace singletons {

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

}  // namespace singletons
}  // namespace chatterino
