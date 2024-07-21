#pragma once

#include "providers/emoji/Emojis.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "singletons/helper/GifTimer.hpp"

namespace chatterino {

class IEmotes
{
public:
    virtual ~IEmotes() = default;

    virtual ITwitchEmotes *getTwitchEmotes() = 0;
    virtual IEmojis *getEmojis() = 0;
    virtual GIFTimer &getGIFTimer() = 0;
};

class Emotes final : public IEmotes
{
public:
    Emotes();

    ITwitchEmotes *getTwitchEmotes() final
    {
        return &this->twitch;
    }

    IEmojis *getEmojis() final
    {
        return &this->emojis;
    }

    GIFTimer &getGIFTimer() final
    {
        return this->gifTimer;
    }

    TwitchEmotes twitch;
    Emojis emojis;

    GIFTimer gifTimer;
};

}  // namespace chatterino
