#pragma once

#include "mocks/Channel.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
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

    const IndirectChannel &getWatchingChannel() const override
    {
        return this->watchingChannel;
    }

    ChannelPtr watchingChannelInner;
    IndirectChannel watchingChannel;
};

}  // namespace chatterino::mock
