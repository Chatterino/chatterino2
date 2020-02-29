#pragma once

#include <IrcMessage>
#include <functional>
#include <mutex>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

#include "common/Common.hpp"
#include "providers/irc/IrcConnection2.hpp"

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class AbstractIrcServer : public QObject
{
public:
    enum ConnectionType { Read = 1, Write = 2, Both = 3 };

    virtual ~AbstractIrcServer() = default;

    // connection
    void connect();
    void disconnect();

    void sendMessage(const QString &channelName, const QString &message);
    void sendRawMessage(const QString &rawMessage);

    // channels
    ChannelPtr getOrAddChannel(const QString &dirtyChannelName);
    ChannelPtr getChannelOrEmpty(const QString &dirtyChannelName);
    std::vector<std::weak_ptr<Channel>> getChannels();

    // signals
    pajlada::Signals::NoArgSignal connected;
    pajlada::Signals::NoArgSignal disconnected;

    void addFakeMessage(const QString &data);

    // iteration
    void forEachChannel(std::function<void(ChannelPtr)> func);

protected:
    AbstractIrcServer();

    // initializeConnectionSignals is called three times.
    // 1. writeConnection with type write
    // 2. readConnection with type read
    // 3. readConnection with type both
    // this can be cleaner once we pass the "connection type" variable into the ctor
    virtual void initializeConnectionSignals(IrcConnection *connection,
                                             ConnectionType type){};
    virtual void initializeConnection(IrcConnection *connection,
                                      ConnectionType type) = 0;
    virtual std::shared_ptr<Channel> createChannel(
        const QString &channelName) = 0;

    virtual void privateMessageReceived(Communi::IrcPrivateMessage *message);
    virtual void readConnectionMessageReceived(Communi::IrcMessage *message);
    virtual void writeConnectionMessageReceived(Communi::IrcMessage *message);

    virtual void onReadConnected(IrcConnection *connection);
    virtual void onWriteConnected(IrcConnection *connection);
    virtual void onDisconnected();
    virtual void onSocketError();

    virtual std::shared_ptr<Channel> getCustomChannel(
        const QString &channelName);

    virtual bool hasSeparateWriteConnection() const = 0;
    virtual QString cleanChannelName(const QString &dirtyChannelName);

    void open(ConnectionType type);

    QMap<QString, std::weak_ptr<Channel>> channels;
    std::mutex channelMutex;

private:
    void initConnection();

    QObjectPtr<IrcConnection> writeConnection_ = nullptr;
    QObjectPtr<IrcConnection> readConnection_ = nullptr;

    QTimer reconnectTimer_;
    int falloffCounter_ = 1;

    std::mutex connectionMutex_;

    //    bool autoReconnect_ = false;
    pajlada::Signals::SignalHolder connections_;
};

}  // namespace chatterino
