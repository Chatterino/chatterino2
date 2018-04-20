#pragma once

#include "providers/irc/abstractircserver.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchchannel.hpp"

#include <memory>

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
    IndirectChannel watchingChannel;

protected:
    void initializeConnection(Communi::IrcConnection *connection, bool isRead,
                              bool isWrite) override;
    std::shared_ptr<Channel> createChannel(const QString &channelName) override;

    void privateMessageReceived(Communi::IrcPrivateMessage *message) override;
    void messageReceived(Communi::IrcMessage *message) override;
    void writeConnectionMessageReceived(Communi::IrcMessage *message) override;

    std::shared_ptr<Channel> getCustomChannel(const QString &channelname) override;

    QString CleanChannelName(const QString &dirtyChannelName) override;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
