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
        , liveChannel(std::shared_ptr<Channel>(new MockChannel("forsen")))
        , automodChannel(std::shared_ptr<Channel>(new MockChannel("forsen2")))
    {
    }

    const IndirectChannel &getWatchingChannel() const override
    {
        return this->watchingChannel;
    }

    QString getLastUserThatWhisperedMe() const override
    {
        return this->lastUserThatWhisperedMe;
    }

    ChannelPtr getLiveChannel() const override
    {
        return this->liveChannel;
    }

    ChannelPtr getAutomodChannel() const override
    {
        return this->automodChannel;
    }

    ChannelPtr watchingChannelInner;
    IndirectChannel watchingChannel;
    ChannelPtr liveChannel;
    ChannelPtr automodChannel;
    QString lastUserThatWhisperedMe{"forsen"};
};

}  // namespace chatterino::mock
