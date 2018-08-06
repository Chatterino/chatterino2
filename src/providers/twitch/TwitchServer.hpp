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

class Settings;
class Paths;

class PubSub;

class TwitchServer final : public AbstractIrcServer, public Singleton
{
public:
    TwitchServer();
    virtual ~TwitchServer() override = default;

    virtual void initialize(Settings &settings, Paths &paths) override;

    void forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func);

    std::shared_ptr<Channel> getChannelOrEmptyByID(const QString &channelID);

    MutexValue<QString> lastUserThatWhisperedMe;

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    IndirectChannel watchingChannel;

    PubSub *pubsub;

protected:
    virtual void initializeConnection(IrcConnection *connection, bool isRead,
                                      bool isWrite) override;
    virtual std::shared_ptr<Channel> createChannel(
        const QString &channelName) override;

    virtual void privateMessageReceived(
        Communi::IrcPrivateMessage *message) override;
    virtual void messageReceived(Communi::IrcMessage *message) override;
    virtual void writeConnectionMessageReceived(
        Communi::IrcMessage *message) override;

    virtual std::shared_ptr<Channel> getCustomChannel(
        const QString &channelname) override;

    virtual QString cleanChannelName(const QString &dirtyChannelName) override;
    virtual bool hasSeparateWriteConnection() const override;

private:
    void onMessageSendRequested(TwitchChannel *channel, const QString &message,
                                bool &sent);

    std::mutex lastMessageMutex_;
    std::queue<std::chrono::steady_clock::time_point> lastMessagePleb_;
    std::queue<std::chrono::steady_clock::time_point> lastMessageMod_;
    std::chrono::steady_clock::time_point lastErrorTimeSpeed_;
    std::chrono::steady_clock::time_point lastErrorTimeAmount_;

    bool singleConnection_ = false;

    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
