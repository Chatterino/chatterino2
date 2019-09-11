#pragma once

#include "providers/irc/IrcConnection2.hpp"

#include <IrcMessage>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

#include <functional>
#include <mutex>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class AbstractIrcServer : public QObject
{
public:
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

    virtual void initializeConnection(IrcConnection *connection, bool isRead,
                                      bool isWrite) = 0;
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

    QMap<QString, std::weak_ptr<Channel>> channels;
    std::mutex channelMutex;

private:
    void initConnection();

    struct Deleter {
        void operator()(IrcConnection *conn)
        {
            conn->deleteLater();
        }
    };

    std::unique_ptr<IrcConnection, Deleter> writeConnection_ = nullptr;
    std::unique_ptr<IrcConnection, Deleter> readConnection_ = nullptr;

    QTimer reconnectTimer_;
    int falloffCounter_ = 1;

    std::mutex connectionMutex_;

    //    bool autoReconnect_ = false;
    pajlada::Signals::SignalHolder connections_;
};

}  // namespace chatterino
