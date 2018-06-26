#pragma once

#include "common/MutexValue.hpp"
#include "providers/irc/AbstractIrcServer.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <chrono>
#include <memory>
#include <queue>

namespace chatterino {

class TwitchServer final : public AbstractIrcServer
{
public:
    TwitchServer();

    void initialize();

    // fourtf: ugh
    void forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func);

    std::shared_ptr<Channel> getChannelOrEmptyByID(const QString &channelID);

    MutexValue<QString> lastUserThatWhisperedMe;

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    IndirectChannel watchingChannel;

protected:
    void initializeConnection(IrcConnection *connection, bool isRead, bool isWrite) override;
    std::shared_ptr<Channel> createChannel(const QString &channelName) override;

    void privateMessageReceived(Communi::IrcPrivateMessage *message) override;
    void messageReceived(Communi::IrcMessage *message) override;
    void writeConnectionMessageReceived(Communi::IrcMessage *message) override;

    std::shared_ptr<Channel> getCustomChannel(const QString &channelname) override;

    QString cleanChannelName(const QString &dirtyChannelName) override;

private:
    std::mutex lastMessageMutex_;
    std::queue<std::chrono::steady_clock::time_point> lastMessagePleb_;
    std::queue<std::chrono::steady_clock::time_point> lastMessageMod_;
    std::chrono::steady_clock::time_point lastErrorTimeSpeed_;
    std::chrono::steady_clock::time_point lastErrorTimeAmount_;

    void onMessageSendRequested(TwitchChannel *channel, const QString &message, bool &sent);
};

}  // namespace chatterino
