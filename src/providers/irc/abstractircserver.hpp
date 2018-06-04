#pragma once

#include "channel.hpp"

#include <IrcConnection>
#include <IrcMessage>
#include <pajlada/signals/signal.hpp>

#include <functional>
#include <mutex>

namespace chatterino {
namespace providers {
namespace irc {

class AbstractIrcServer
{
public:
    virtual ~AbstractIrcServer() = default;

    // connection
    Communi::IrcConnection *getReadConnection() const;

    void connect();
    void disconnect();

    void sendMessage(const QString &channelName, const QString &message);

    // channels
    std::shared_ptr<Channel> getOrAddChannel(const QString &dirtyChannelName);
    std::shared_ptr<Channel> getChannelOrEmpty(const QString &dirtyChannelName);

    // signals
    pajlada::Signals::NoArgSignal connected;
    pajlada::Signals::NoArgSignal disconnected;
    //    pajlada::Signals::Signal<Communi::IrcPrivateMessage *> onPrivateMessage;

    void addFakeMessage(const QString &data);

    // iteration
    void forEachChannel(std::function<void(ChannelPtr)> func);

protected:
    AbstractIrcServer();

    virtual void initializeConnection(Communi::IrcConnection *connection, bool isRead,
                                      bool isWrite) = 0;
    virtual std::shared_ptr<Channel> createChannel(const QString &channelName) = 0;

    virtual void privateMessageReceived(Communi::IrcPrivateMessage *message);
    virtual void messageReceived(Communi::IrcMessage *message);
    virtual void writeConnectionMessageReceived(Communi::IrcMessage *message);

    virtual void onConnected();
    virtual void onDisconnected();

    virtual std::shared_ptr<Channel> getCustomChannel(const QString &channelName);

    virtual QString cleanChannelName(const QString &dirtyChannelName);

    QMap<QString, std::weak_ptr<Channel>> channels;
    std::mutex channelMutex;

private:
    void initConnection();

    std::unique_ptr<Communi::IrcConnection> writeConnection = nullptr;
    std::unique_ptr<Communi::IrcConnection> readConnection = nullptr;

    std::mutex connectionMutex;
};

}  // namespace irc
}  // namespace providers
}  // namespace chatterino
