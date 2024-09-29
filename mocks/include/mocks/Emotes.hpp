#pragma once

#include "singletons/Emotes.hpp"

namespace chatterino::mock {

class Emotes : public IEmotes
{
public:
    Emotes()
    {
        this->emojis.load();
        // don't initialize GIFTimer
    }

    ITwitchEmotes *getTwitchEmotes() override
    {
        return &this->twitch;
    }

    IEmojis *getEmojis() override
    {
        return &this->emojis;
    }

    GIFTimer &getGIFTimer() override
    {
        return this->gifTimer;
    }

private:
    TwitchEmotes twitch;
    Emojis emojis;

    GIFTimer gifTimer;
};

}  // namespace chatterino::mock
