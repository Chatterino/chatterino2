#pragma once

#include "mocks/Channel.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/eventapi/Client.hpp"
#include "providers/seventv/eventapi/Dispatch.hpp"
#include "providers/seventv/eventapi/Message.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "util/Twitch.hpp"

#include <unordered_map>

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

    void connect() override
    {
    }

    void sendRawMessage(const QString &rawMessage) override
    {
    }

    ChannelPtr getOrAddChannel(const QString &dirtyChannelName) override
    {
        assert(false && "unimplemented getOrAddChannel in mock irc server");
        return {};
    }

    ChannelPtr getChannelOrEmpty(const QString &dirtyChannelName) override
    {
        QString query = cleanChannelName(dirtyChannelName);

        auto it = this->mockChannels.find(query);
        if (it == this->mockChannels.end())
        {
            return Channel::getEmpty();
        }
        auto chan = it->second.lock();
        if (!chan)
        {
            return Channel::getEmpty();
        }
        return chan;
    }

    void addFakeMessage(const QString &data) override
    {
    }

    void addGlobalSystemMessage(const QString &messageText) override
    {
    }

    void forEachChannel(std::function<void(ChannelPtr)> func) override
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
        // XXX: this is the same as in TwitchIrcServer::getChannelOrEmptyByID
        for (const auto &[name, weakChannel] : this->mockChannels)
        {
            auto channel = weakChannel.lock();
            if (!channel)
            {
                continue;
            }

            auto twitchChannel =
                std::dynamic_pointer_cast<TwitchChannel>(channel);
            if (!twitchChannel)
            {
                continue;
            }

            if (twitchChannel->roomId() == channelID &&
                twitchChannel->getName().count(':') < 2)
            {
                return channel;
            }
        }

        return Channel::getEmpty();
    }

    void dropSeventvChannel(const QString &userID,
                            const QString &emoteSetID) override
    {
        //
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

    std::unordered_map<QString, std::weak_ptr<Channel>> mockChannels;
};

}  // namespace chatterino::mock
