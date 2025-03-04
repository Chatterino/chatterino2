#pragma once

#include "common/Atomic.hpp"
#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "providers/irc/IrcConnection2.hpp"
#include "util/RatelimitBucket.hpp"

#include <IrcMessage>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>

namespace chatterino {

class Settings;
class Paths;
class TwitchChannel;
class BttvEmotes;
class FfzEmotes;
class SeventvEmotes;
class RatelimitBucket;

class ITwitchIrcServer
{
public:
    ITwitchIrcServer() = default;
    virtual ~ITwitchIrcServer() = default;
    ITwitchIrcServer(const ITwitchIrcServer &) = delete;
    ITwitchIrcServer(ITwitchIrcServer &&) = delete;
    ITwitchIrcServer &operator=(const ITwitchIrcServer &) = delete;
    ITwitchIrcServer &operator=(ITwitchIrcServer &&) = delete;

    virtual void connect() = 0;

    virtual void sendRawMessage(const QString &rawMessage) = 0;

    virtual ChannelPtr getOrAddChannel(const QString &dirtyChannelName) = 0;
    virtual ChannelPtr getChannelOrEmpty(const QString &dirtyChannelName) = 0;

    virtual void addFakeMessage(const QString &data) = 0;

    virtual void addGlobalSystemMessage(const QString &messageText) = 0;

    virtual void forEachChannel(std::function<void(ChannelPtr)> func) = 0;

    virtual void forEachChannelAndSpecialChannels(
        std::function<void(ChannelPtr)> func) = 0;

    virtual std::shared_ptr<Channel> getChannelOrEmptyByID(
        const QString &channelID) = 0;

    virtual void dropSeventvChannel(const QString &userID,
                                    const QString &emoteSetID) = 0;

    virtual const IndirectChannel &getWatchingChannel() const = 0;
    virtual void setWatchingChannel(ChannelPtr newWatchingChannel) = 0;
    virtual ChannelPtr getWhispersChannel() const = 0;
    virtual ChannelPtr getMentionsChannel() const = 0;
    virtual ChannelPtr getLiveChannel() const = 0;
    virtual ChannelPtr getAutomodChannel() const = 0;

    virtual QString getLastUserThatWhisperedMe() const = 0;
    virtual void setLastUserThatWhisperedMe(const QString &user) = 0;

    // Update this interface with TwitchIrcServer methods as needed
};

class TwitchIrcServer final : public ITwitchIrcServer, public QObject
{
public:
    enum class ConnectionType {
        Read,
        Write,
    };

    TwitchIrcServer();
    ~TwitchIrcServer() override = default;

    TwitchIrcServer(const TwitchIrcServer &) = delete;
    TwitchIrcServer(TwitchIrcServer &&) = delete;
    TwitchIrcServer &operator=(const TwitchIrcServer &) = delete;
    TwitchIrcServer &operator=(TwitchIrcServer &&) = delete;

    void initialize();

    void forEachChannelAndSpecialChannels(
        std::function<void(ChannelPtr)> func) override;

    std::shared_ptr<Channel> getChannelOrEmptyByID(
        const QString &channelID) override;

    void reloadAllBTTVChannelEmotes();
    void reloadAllFFZChannelEmotes();
    void reloadAllSevenTVChannelEmotes();

    /** Calls `func` with all twitch channels that have `emoteSetId` added. */
    void forEachSeventvEmoteSet(const QString &emoteSetId,
                                std::function<void(TwitchChannel &)> func);
    /** Calls `func` with all twitch channels where the seventv-user-id is `userId`. */
    void forEachSeventvUser(const QString &userId,
                            std::function<void(TwitchChannel &)> func);
    /**
     * Checks if any channel still needs this `userID` or `emoteSetID`.
     * If not, it unsubscribes from the respective messages.
     *
     * It's currently not possible to share emote sets among users,
     * but it's a commonly requested feature.
     */
    void dropSeventvChannel(const QString &userID,
                            const QString &emoteSetID) override;

    void addFakeMessage(const QString &data) override;

    void addGlobalSystemMessage(const QString &messageText) override;

    // iteration
    void forEachChannel(std::function<void(ChannelPtr)> func) override;

    void connect() override;
    void disconnect();

    void sendMessage(const QString &channelName, const QString &message);
    void sendRawMessage(const QString &rawMessage) override;

    ChannelPtr getOrAddChannel(const QString &dirtyChannelName) override;

    ChannelPtr getChannelOrEmpty(const QString &dirtyChannelName) override;

    void open(ConnectionType type);

private:
    Atomic<QString> lastUserThatWhisperedMe;

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    const ChannelPtr liveChannel;
    const ChannelPtr automodChannel;
    IndirectChannel watchingChannel;

public:
    const IndirectChannel &getWatchingChannel() const override;
    void setWatchingChannel(ChannelPtr newWatchingChannel) override;
    ChannelPtr getWhispersChannel() const override;
    ChannelPtr getMentionsChannel() const override;
    ChannelPtr getLiveChannel() const override;
    ChannelPtr getAutomodChannel() const override;

    QString getLastUserThatWhisperedMe() const override;
    void setLastUserThatWhisperedMe(const QString &user) override;

protected:
    void initializeConnection(IrcConnection *connection, ConnectionType type);
    std::shared_ptr<Channel> createChannel(const QString &channelName);

    void privateMessageReceived(Communi::IrcPrivateMessage *message);
    void readConnectionMessageReceived(Communi::IrcMessage *message);
    void writeConnectionMessageReceived(Communi::IrcMessage *message);

    void onReadConnected(IrcConnection *connection);
    void onWriteConnected(IrcConnection *connection);
    void onDisconnected();
    void markChannelsConnected();

    std::shared_ptr<Channel> getCustomChannel(const QString &channelname);

private:
    void onMessageSendRequested(const std::shared_ptr<TwitchChannel> &channel,
                                const QString &message, bool &sent);
    void onReplySendRequested(const std::shared_ptr<TwitchChannel> &channel,
                              const QString &message, const QString &replyId,
                              bool &sent);

    bool prepareToSend(const std::shared_ptr<TwitchChannel> &channel);

    QMap<QString, std::weak_ptr<Channel>> channels;
    std::mutex channelMutex;

    QObjectPtr<IrcConnection> writeConnection_ = nullptr;
    QObjectPtr<IrcConnection> readConnection_ = nullptr;

    // Our rate limiting bucket for the Twitch join rate limits
    // https://dev.twitch.tv/docs/irc/guide#rate-limits
    QObjectPtr<RatelimitBucket> joinBucket_;

    QTimer reconnectTimer_;
    int falloffCounter_ = 1;

    std::mutex connectionMutex_;

    pajlada::Signals::SignalHolder connections_;

    std::mutex lastMessageMutex_;
    std::queue<std::chrono::steady_clock::time_point> lastMessagePleb_;
    std::queue<std::chrono::steady_clock::time_point> lastMessageMod_;
    std::chrono::steady_clock::time_point lastErrorTimeSpeed_;
    std::chrono::steady_clock::time_point lastErrorTimeAmount_;
};

}  // namespace chatterino
