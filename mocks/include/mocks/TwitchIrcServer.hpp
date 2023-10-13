#pragma once

#include "mocks/Channel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"

namespace chatterino::mock {

class MockTwitchIrcServer : public ITwitchIrcServer
{
public:
    MockTwitchIrcServer()
        : watchingChannelInner(
              std::shared_ptr<Channel>(new MockChannel("testaccount_420")))
        , watchingChannel(this->watchingChannelInner,
                          Channel::Type::TwitchWatching)
    {
    }

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

    const IndirectChannel &getWatchingChannel() const override
    {
        return this->watchingChannel;
    }

    BttvEmotes bttv;
    FfzEmotes ffz;
    SeventvEmotes seventv;
    ChannelPtr watchingChannelInner;
    IndirectChannel watchingChannel;
};

}  // namespace chatterino::mock
