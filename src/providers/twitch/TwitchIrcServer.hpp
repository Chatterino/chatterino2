#pragma once

#include "common/Atomic.hpp"
#include "common/Channel.hpp"
#include "common/Singleton.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/irc/AbstractIrcServer.hpp"
#include "providers/seventv/SeventvEmotes.hpp"

#include <pajlada/signals/signalholder.hpp>

#include <chrono>
#include <memory>
#include <queue>

namespace chatterino {

class Settings;
class Paths;
class PubSub;
class TwitchChannel;
class BttvLiveUpdates;
class SeventvEventAPI;

class ITwitchIrcServer
{
public:
    virtual ~ITwitchIrcServer() = default;

    virtual const BttvEmotes &getBttvEmotes() const = 0;
    virtual const FfzEmotes &getFfzEmotes() const = 0;
    virtual const SeventvEmotes &getSeventvEmotes() const = 0;
    virtual const IndirectChannel &getWatchingChannel() const = 0;

    // Update this interface with TwitchIrcServer methods as needed
};

class TwitchIrcServer final : public AbstractIrcServer,
                              public Singleton,
                              public ITwitchIrcServer
{
public:
    TwitchIrcServer();
    ~TwitchIrcServer() override = default;

    void initialize(Settings &settings, Paths &paths) override;

    void forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func);

    std::shared_ptr<Channel> getChannelOrEmptyByID(const QString &channelID);

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
    void dropSeventvChannel(const QString &userID, const QString &emoteSetID);

    Atomic<QString> lastUserThatWhisperedMe;

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    const ChannelPtr liveChannel;
    const ChannelPtr automodChannel;
    IndirectChannel watchingChannel;

    // NOTE: We currently leak this
    PubSub *pubsub;
    std::unique_ptr<BttvLiveUpdates> bttvLiveUpdates;
    std::unique_ptr<SeventvEventAPI> seventvEventAPI;

    const BttvEmotes &getBttvEmotes() const override;
    const FfzEmotes &getFfzEmotes() const override;
    const SeventvEmotes &getSeventvEmotes() const override;
    const IndirectChannel &getWatchingChannel() const override;

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
    void onMessageSendRequested(TwitchChannel *channel, const QString &message,
                                bool &sent);
    void onReplySendRequested(TwitchChannel *channel, const QString &message,
                              const QString &replyId, bool &sent);

    bool prepareToSend(TwitchChannel *channel);

    std::mutex lastMessageMutex_;
    std::queue<std::chrono::steady_clock::time_point> lastMessagePleb_;
    std::queue<std::chrono::steady_clock::time_point> lastMessageMod_;
    std::chrono::steady_clock::time_point lastErrorTimeSpeed_;
    std::chrono::steady_clock::time_point lastErrorTimeAmount_;

    BttvEmotes bttv;
    FfzEmotes ffz;
    SeventvEmotes seventv_;

    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
