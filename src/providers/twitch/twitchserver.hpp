#pragma once

#include <memory>

#include "providers/irc/abstractircserver.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchchannel.hpp"

namespace chatterino {
namespace providers {
namespace twitch {
class TwitchServer final : public irc::AbstractIrcServer
{
    TwitchServer();

public:
    static TwitchServer &getInstance();

    // fourtf: ugh
    void forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func);

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;

protected:
    virtual void initializeConnection(Communi::IrcConnection *connection, bool isRead,
                                      bool isWrite) override;
    virtual std::shared_ptr<Channel> createChannel(const QString &channelName) override;

    virtual void privateMessageReceived(Communi::IrcPrivateMessage *message) override;
    virtual void messageReceived(Communi::IrcMessage *message) override;
    virtual void writeConnectionMessageReceived(Communi::IrcMessage *message) override;

    virtual std::shared_ptr<Channel> getCustomChannel(const QString &channelname) override;
};
}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
