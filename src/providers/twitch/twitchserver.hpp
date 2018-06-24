#pragma once

#include "providers/irc/abstractircserver.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchchannel.hpp"
#include "util/mutexvalue.hpp"

#include <chrono>
#include <memory>
#include <queue>

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchServer final : public irc::AbstractIrcServer
{
public:
    TwitchServer();

    void initialize();

    // fourtf: ugh
    void forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func);

    std::shared_ptr<Channel> getChannelOrEmptyByID(const QString &channelID);

    util::MutexValue<QString> lastUserThatWhisperedMe;

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    IndirectChannel watchingChannel;

protected:
    void initializeConnection(providers::irc::IrcConnection *connection, bool isRead,
                              bool isWrite) override;
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

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
