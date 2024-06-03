#pragma once

#include "common/Atomic.hpp"
#include "common/Channel.hpp"
#include "providers/irc/AbstractIrcServer.hpp"

#include <pajlada/signals/signalholder.hpp>

#include <chrono>
#include <memory>
#include <queue>

namespace chatterino {

class Settings;
class Paths;
class TwitchChannel;
class BttvLiveUpdates;
class SeventvEventAPI;
class BttvEmotes;
class FfzEmotes;
class SeventvEmotes;

class ITwitchIrcServer
{
public:
    virtual ~ITwitchIrcServer() = default;

    virtual void forEachChannelAndSpecialChannels(
        std::function<void(ChannelPtr)> func) = 0;

    virtual std::shared_ptr<Channel> getChannelOrEmptyByID(
        const QString &channelID) = 0;

    virtual void dropSeventvChannel(const QString &userID,
                                    const QString &emoteSetID) = 0;

    virtual std::unique_ptr<BttvLiveUpdates> &getBTTVLiveUpdates() = 0;
    virtual std::unique_ptr<SeventvEventAPI> &getSeventvEventAPI() = 0;

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

class TwitchIrcServer final : public AbstractIrcServer, public ITwitchIrcServer
{
public:
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

    void reloadBTTVGlobalEmotes();
    void reloadAllBTTVChannelEmotes();
    void reloadFFZGlobalEmotes();
    void reloadAllFFZChannelEmotes();
    void reloadSevenTVGlobalEmotes();
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

private:
    Atomic<QString> lastUserThatWhisperedMe;

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    const ChannelPtr liveChannel;
    const ChannelPtr automodChannel;
    IndirectChannel watchingChannel;

    std::unique_ptr<BttvLiveUpdates> bttvLiveUpdates;
    std::unique_ptr<SeventvEventAPI> seventvEventAPI;

public:
    std::unique_ptr<BttvLiveUpdates> &getBTTVLiveUpdates() override;
    std::unique_ptr<SeventvEventAPI> &getSeventvEventAPI() override;

    const IndirectChannel &getWatchingChannel() const override;
    void setWatchingChannel(ChannelPtr newWatchingChannel) override;
    ChannelPtr getWhispersChannel() const override;
    ChannelPtr getMentionsChannel() const override;
    ChannelPtr getLiveChannel() const override;
    ChannelPtr getAutomodChannel() const override;

    QString getLastUserThatWhisperedMe() const override;
    void setLastUserThatWhisperedMe(const QString &user) override;

protected:
    void initializeConnection(IrcConnection *connection,
                              ConnectionType type) override;
    std::shared_ptr<Channel> createChannel(const QString &channelName) override;

    void privateMessageReceived(Communi::IrcPrivateMessage *message) override;
    void readConnectionMessageReceived(Communi::IrcMessage *message) override;
    void writeConnectionMessageReceived(Communi::IrcMessage *message) override;

    std::shared_ptr<Channel> getCustomChannel(
        const QString &channelname) override;

    QString cleanChannelName(const QString &dirtyChannelName) override;
    bool hasSeparateWriteConnection() const override;

private:
    void onMessageSendRequested(const std::shared_ptr<TwitchChannel> &channel,
                                const QString &message, bool &sent);
    void onReplySendRequested(const std::shared_ptr<TwitchChannel> &channel,
                              const QString &message, const QString &replyId,
                              bool &sent);

    bool prepareToSend(const std::shared_ptr<TwitchChannel> &channel);

    std::mutex lastMessageMutex_;
    std::queue<std::chrono::steady_clock::time_point> lastMessagePleb_;
    std::queue<std::chrono::steady_clock::time_point> lastMessageMod_;
    std::chrono::steady_clock::time_point lastErrorTimeSpeed_;
    std::chrono::steady_clock::time_point lastErrorTimeAmount_;

    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
