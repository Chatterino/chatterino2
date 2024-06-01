#pragma once

#include "mocks/Channel.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/bttv/BttvLiveUpdates.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/eventapi/Client.hpp"
#include "providers/seventv/eventapi/Dispatch.hpp"
#include "providers/seventv/eventapi/Message.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/seventv/SeventvEventAPI.hpp"
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
        , whispersChannel(std::shared_ptr<Channel>(new MockChannel("whispers")))
        , mentionsChannel(std::shared_ptr<Channel>(new MockChannel("forsen3")))
        , liveChannel(std::shared_ptr<Channel>(new MockChannel("forsen")))
        , automodChannel(std::shared_ptr<Channel>(new MockChannel("forsen2")))
    {
    }

    void forEachChannelAndSpecialChannels(
        std::function<void(ChannelPtr)> func) override
    {
        //
    }

    std::shared_ptr<Channel> getChannelOrEmptyByID(
        const QString &channelID) override
    {
        return {};
    }

    void dropSeventvChannel(const QString &userID,
                            const QString &emoteSetID) override
    {
        //
    }

    std::unique_ptr<BttvLiveUpdates> &getBTTVLiveUpdates() override
    {
        return this->bttvLiveUpdates;
    }

    std::unique_ptr<SeventvEventAPI> &getSeventvEventAPI() override
    {
        return this->seventvEventAPI;
    }

    const IndirectChannel &getWatchingChannel() const override
    {
        return this->watchingChannel;
    }

    void setWatchingChannel(ChannelPtr newWatchingChannel) override
    {
        this->watchingChannel.reset(newWatchingChannel);
    }

    QString getLastUserThatWhisperedMe() const override
    {
        return this->lastUserThatWhisperedMe;
    }

    void setLastUserThatWhisperedMe(const QString &user) override
    {
        this->lastUserThatWhisperedMe = user;
    }

    ChannelPtr getWhispersChannel() const override
    {
        return this->whispersChannel;
    }

    ChannelPtr getMentionsChannel() const override
    {
        return this->mentionsChannel;
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
    ChannelPtr whispersChannel;
    ChannelPtr mentionsChannel;
    ChannelPtr liveChannel;
    ChannelPtr automodChannel;
    QString lastUserThatWhisperedMe{"forsen"};

    std::unique_ptr<BttvLiveUpdates> bttvLiveUpdates;
    std::unique_ptr<SeventvEventAPI> seventvEventAPI;
};

}  // namespace chatterino::mock
