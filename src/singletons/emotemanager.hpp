#pragma once

#define GIF_FRAME_LENGTH 33

#include "messages/image.hpp"
#include "providers/bttv/bttvemotes.hpp"
#include "providers/emoji/emojis.hpp"
#include "providers/ffz/ffzemotes.hpp"
#include "providers/twitch/twitchemotes.hpp"
#include "singletons/helper/giftimer.hpp"
#include "util/concurrentmap.hpp"
#include "util/emotemap.hpp"

#include <QString>

namespace chatterino {
namespace singletons {

class EmoteManager
{
public:
    ~EmoteManager() = delete;

    providers::twitch::TwitchEmotes twitch;
    providers::bttv::BTTVEmotes bttv;
    providers::ffz::FFZEmotes ffz;
    providers::emoji::Emojis emojis;

    GIFTimer gifTimer;

    void initialize();

    util::EmoteMap &getChatterinoEmotes();

    util::EmoteData getCheerImage(long long int amount, bool animated);

    // Bit badge/emotes?
    // TODO: Move to twitch emote provider
    util::ConcurrentMap<QString, messages::Image *> miscImageCache;

private:
    /// Chatterino emotes
    util::EmoteMap _chatterinoEmotes;
};

}  // namespace singletons
}  // namespace chatterino
