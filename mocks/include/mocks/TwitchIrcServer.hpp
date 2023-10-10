#pragma once

#include "providers/twitch/TwitchIrcServer.hpp"

namespace chatterino::mock {

class MockTwitchIrcServer : public ITwitchIrcServer
{
public:
    const BttvEmotes &getBttvEmotes() const override
    {
        return this->bttv;
    }

    const FfzEmotes &getFfzEmotes() const override
    {
        return this->ffz;
    }

    const SeventvEmotes &getSeventvEmotes() const override
    {
        return this->seventv;
    }

    BttvEmotes bttv;
    FfzEmotes ffz;
    SeventvEmotes seventv;
};

}  // namespace chatterino::mock
