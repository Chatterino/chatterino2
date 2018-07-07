#pragma once

#include "common/MutexValue.hpp"
#include "common/Singleton.hpp"
#include "providers/irc/AbstractIrcServer.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <chrono>
#include <memory>
#include <queue>

namespace chatterino {

class PubSub;

class TwitchServer : public AbstractIrcServer, public Singleton
{
public:
    TwitchServer();
    virtual ~TwitchServer() override = default;

    virtual void initialize(Application &app) override;

    // fourtf: ugh
    void forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func);

    std::shared_ptr<Channel> getChannelOrEmptyByID(const QString &channelID);

    MutexValue<QString> lastUserThatWhisperedMe;

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    IndirectChannel watchingChannel;

    PubSub *pubsub;

protected:
    void initializeConnection(IrcConnection *connection, bool isRead, bool isWrite) override;
    std::shared_ptr<Channel> createChannel(const QString &channelName) override;

    void privateMessageReceived(Communi::IrcPrivateMessage *message) override;
    void messageReceived(Communi::IrcMessage *message) override;
    void writeConnectionMessageReceived(Communi::IrcMessage *message) override;

    std::shared_ptr<Channel> getCustomChannel(const QString &channelname) override;

    QString cleanChannelName(const QString &dirtyChannelName) override;

private:
    void onMessageSendRequested(TwitchChannel *channel, const QString &message, bool &sent);

    Application *app = nullptr;

    std::mutex lastMessageMutex_;
    std::queue<std::chrono::steady_clock::time_point> lastMessagePleb_;
    std::queue<std::chrono::steady_clock::time_point> lastMessageMod_;
    std::chrono::steady_clock::time_point lastErrorTimeSpeed_;
    std::chrono::steady_clock::time_point lastErrorTimeAmount_;
};

}  // namespace chatterino
