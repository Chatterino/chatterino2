#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "common/Channel.hpp"
#include "common/ChannelChatters.hpp"
#include "common/Common.hpp"
#include "common/UniqueAccess.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "util/QStringHash.hpp"
#include "util/ThreadGuard.hpp"

#include <boost/circular_buffer/space_optimized.hpp>
#include <boost/signals2.hpp>
#include <IrcMessage>
#include <pajlada/signals/signalholder.hpp>
#include <QColor>
#include <QElapsedTimer>
#include <QRegularExpression>

#include <atomic>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace chatterino {

// This is for compatibility with older Chatterino versions. Twitch didn't use
// to allow ZERO WIDTH JOINER unicode character, so Chatterino used ESCAPE_TAG
// instead.
// See https://github.com/Chatterino/chatterino2/issues/3384 and
// https://mm2pl.github.io/emoji_rfc.pdf for more details
const QString ZERO_WIDTH_JOINER = QString(QChar(0x200D));

// Here be MSVC: Do NOT replace with "\U" literal, it will fail silently.
namespace {
    const QChar ESCAPE_TAG_CHARS[2] = {QChar::highSurrogate(0xE0002),
                                       QChar::lowSurrogate(0xE0002)};
}
const QString ESCAPE_TAG = QString(ESCAPE_TAG_CHARS, 2);

const static QRegularExpression COMBINED_FIXER(
    QString("(?<!%1)%1").arg(ESCAPE_TAG),
    QRegularExpression::UseUnicodePropertiesOption);

enum class HighlightState;

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;

class TwitchBadges;
class FfzEmotes;
class BttvEmotes;
struct BttvLiveUpdateEmoteUpdateAddMessage;
struct BttvLiveUpdateEmoteRemoveMessage;

class SeventvEmotes;
namespace seventv::eventapi {
    struct EmoteAddDispatch;
    struct EmoteUpdateDispatch;
    struct EmoteRemoveDispatch;
    struct UserConnectionUpdateDispatch;
}  // namespace seventv::eventapi

struct ChannelPointReward;
class MessageThread;
struct CheerEmoteSet;
struct HelixStream;

class TwitchIrcServer;

const int MAX_QUEUED_REDEMPTIONS = 16;

class TwitchChannel final : public Channel, public ChannelChatters
{
public:
    struct StreamStatus {
        bool live = false;
        bool rerun = false;
        unsigned viewerCount = 0;
        QString title;
        QString game;
        QString gameId;
        QString uptime;
        int uptimeSeconds = 0;
        QString streamType;
    };

    struct RoomModes {
        bool submode = false;
        bool r9k = false;
        bool emoteOnly = false;

        /**
         * @brief Number of minutes required for users to be followed before typing in chat
         *
         * Special cases:
         * -1 = follower mode off
         *  0 = follower mode on, no time requirement
         **/
        int followerOnly = -1;

        /**
         * @brief Number of seconds required to wait before typing emotes
         *
         * 0 = slow mode off
         **/
        int slowMode = 0;
    };

    explicit TwitchChannel(const QString &channelName);
    ~TwitchChannel() override;

    TwitchChannel(const TwitchChannel &) = delete;
    TwitchChannel(TwitchChannel &&) = delete;
    TwitchChannel &operator=(const TwitchChannel &) = delete;
    TwitchChannel &operator=(TwitchChannel &&) = delete;

    void initialize();

    // Channel methods
    bool isEmpty() const override;
    bool canSendMessage() const override;
    void sendMessage(const QString &message) override;
    void sendReply(const QString &message, const QString &replyId);
    bool isMod() const override;
    bool isVip() const;
    bool isStaff() const;
    bool isBroadcaster() const override;
    bool hasHighRateLimit() const override;
    bool canReconnect() const override;
    void reconnect() override;
    void createClip();

    // Data
    const QString &subscriptionUrl();
    const QString &channelUrl();
    const QString &popoutPlayerUrl();
    int chatterCount() const;
    bool isLive() const override;
    bool isRerun() const override;
    QString roomId() const;
    SharedAccessGuard<const RoomModes> accessRoomModes() const;
    SharedAccessGuard<const StreamStatus> accessStreamStatus() const;

    /**
     * Records that the channel is no longer joined.
     */
    void markDisconnected();

    /**
     * Records that the channel's read connection is healthy.
     */
    void markConnected();

    // Emotes
    std::optional<EmotePtr> bttvEmote(const EmoteName &name) const;
    std::optional<EmotePtr> ffzEmote(const EmoteName &name) const;
    std::optional<EmotePtr> seventvEmote(const EmoteName &name) const;
    std::shared_ptr<const EmoteMap> bttvEmotes() const;
    std::shared_ptr<const EmoteMap> ffzEmotes() const;
    std::shared_ptr<const EmoteMap> seventvEmotes() const;

    void refreshBTTVChannelEmotes(bool manualRefresh);
    void refreshFFZChannelEmotes(bool manualRefresh);
    void refreshSevenTVChannelEmotes(bool manualRefresh);

    void setBttvEmotes(std::shared_ptr<const EmoteMap> &&map);
    void setFfzEmotes(std::shared_ptr<const EmoteMap> &&map);
    void setSeventvEmotes(std::shared_ptr<const EmoteMap> &&map);

    const QString &seventvUserID() const;
    const QString &seventvEmoteSetID() const;

    /** Adds a BTTV channel emote to this channel. */
    void addBttvEmote(const BttvLiveUpdateEmoteUpdateAddMessage &message);
    /** Updates a BTTV channel emote in this channel. */
    void updateBttvEmote(const BttvLiveUpdateEmoteUpdateAddMessage &message);
    /** Removes a BTTV channel emote from this channel. */
    void removeBttvEmote(const BttvLiveUpdateEmoteRemoveMessage &message);

    /** Adds a 7TV channel emote to this channel. */
    void addSeventvEmote(const seventv::eventapi::EmoteAddDispatch &dispatch);
    /** Updates a 7TV channel emote's name in this channel */
    void updateSeventvEmote(
        const seventv::eventapi::EmoteUpdateDispatch &dispatch);
    /** Removes a 7TV channel emote from this channel */
    void removeSeventvEmote(
        const seventv::eventapi::EmoteRemoveDispatch &dispatch);
    /** Updates the current 7TV user. Currently, only the emote-set is updated. */
    void updateSeventvUser(
        const seventv::eventapi::UserConnectionUpdateDispatch &dispatch);

    // Update the channel's 7TV information (the channel's 7TV user ID and emote set ID)
    void updateSeventvData(const QString &newUserID,
                           const QString &newEmoteSetID);

    // Badges
    std::optional<EmotePtr> ffzCustomModBadge() const;
    std::optional<EmotePtr> ffzCustomVipBadge() const;
    std::optional<EmotePtr> twitchBadge(const QString &set,
                                        const QString &version) const;
    /**
     * Returns a list of channel-specific FrankerFaceZ badges for the given user
     */
    std::vector<FfzBadges::Badge> ffzChannelBadges(const QString &userID) const;

    // Cheers
    std::optional<CheerEmote> cheerEmote(const QString &string);

    // Replies
    /**
     * Stores the given thread in this channel. 
     * 
     * Note: This method not take ownership of the MessageThread; this 
     * TwitchChannel instance will store a weak_ptr to the thread.
     */
    void addReplyThread(const std::shared_ptr<MessageThread> &thread);
    const std::unordered_map<QString, std::weak_ptr<MessageThread>> &threads()
        const;

    /**
     * Get the thread for the given message
     * If no thread can be found for the message, create one
     */
    std::shared_ptr<MessageThread> getOrCreateThread(const MessagePtr &message);

    /**
     * This signal fires when the local user has joined the channel
     **/
    pajlada::Signals::NoArgSignal joined;

    // Only TwitchChannel may invoke this signal
    pajlada::Signals::NoArgSignal userStateChanged;

    /**
     * This signals fires whenever the live status is changed
     *
     * Streams are counted as offline by default, so if a stream does not go online
     * this signal will never fire
     **/
    pajlada::Signals::Signal<bool> liveStatusChanged;

    /**
     * This signal fires whenever the stream status is changed
     *
     * This includes when the stream goes from offline to online,
     * or the viewer count changes, or the title has been updated
     **/
    pajlada::Signals::NoArgSignal streamStatusChanged;

    pajlada::Signals::NoArgSignal roomModesChanged;

    // Channel point rewards
    void addQueuedRedemption(const QString &rewardId,
                             const QString &originalContent,
                             Communi::IrcMessage *message);
    /**
     * A rich & hydrated redemption from PubSub has arrived, add it to the channel.
     * This will look at queued up partial messages, and if one is found it will add the queued up partial messages fully hydrated.
     **/
    void addChannelPointReward(const ChannelPointReward &reward);
    bool isChannelPointRewardKnown(const QString &rewardId);
    std::optional<ChannelPointReward> channelPointReward(
        const QString &rewardId) const;

    // Live status
    void updateStreamStatus(const std::optional<HelixStream> &helixStream);
    void updateStreamTitle(const QString &title);

    /**
     * Returns the display name of the user
     *
     * If the display name contained chinese, japenese, or korean characters, the user's login name is returned instead
     **/
    const QString &getDisplayName() const override;
    void updateDisplayName(const QString &displayName);

private:
    struct NameOptions {
        // displayName is the non-CJK-display name for this user
        // This will always be the same as their `name_`, but potentially with different casing
        QString displayName;

        // localizedName is their display name that *may* contain CJK characters
        // If the display name does not contain any CJK characters, this will be
        // the same as `displayName`
        QString localizedName;

        // actualDisplayName is the raw display name string received from Twitch
        QString actualDisplayName;
    } nameOptions;

    struct QueuedRedemption {
        QString rewardID;
        QString originalContent;
        QObjectPtr<Communi::IrcMessage> message;
    };

    void refreshPubSub();
    void refreshChatters();
    void refreshBadges();
    void refreshCheerEmotes();
    void loadRecentMessages();
    void loadRecentMessagesReconnect();
    void cleanUpReplyThreads();
    void showLoginMessage();

    /// roomIdChanged is called whenever this channel's ID has been changed
    /// This should only happen once per channel, whenever the ID goes from unset to set
    void roomIdChanged();

    /** Joins (subscribes to) a Twitch channel for updates on BTTV. */
    void joinBttvChannel() const;
    /**
     * Indicates an activity to 7TV in this channel for this user.
     * This is done at most once every 60s.
     */
    void updateSevenTVActivity();
    void listenSevenTVCosmetics() const;

    /**
     * @brief Sets the live status of this Twitch channel
     *
     * Returns true if the live status changed with this call
     **/
    bool setLive(bool newLiveStatus);
    void setMod(bool value);
    void setVIP(bool value);
    void setStaff(bool value);
    void setRoomId(const QString &id);
    void setRoomModes(const RoomModes &newRoomModes);
    void setDisplayName(const QString &name);
    void setLocalizedName(const QString &name);

    /**
     * Returns the localized name of the user
     **/
    const QString &getLocalizedName() const override;

    QString prepareMessage(const QString &message) const;

    /**
     * Either adds a message mentioning the updated emotes
     * or replaces an existing message. For criteria on existing messages,
     * see `tryReplaceLastLiveUpdateAddOrRemove`.
     *
     * @param isEmoteAdd true if the emote was added, false if it was removed.
     * @param platform The platform the emote was updated on ("7TV", "BTTV", "FFZ")
     * @param actor The actor performing the update (possibly empty)
     * @param emoteName The emote's name
     */
    void addOrReplaceLiveUpdatesAddRemove(bool isEmoteAdd,
                                          const QString &platform,
                                          const QString &actor,
                                          const QString &emoteName);

    /**
     * Tries to replace the last emote update message.
     *
     * A last message is valid if:
     *  * The actors match
     *  * The operations match
     *  * The platform matches
     *  * The last message isn't older than 5s
     *
     * @param op The emote operation (LiveUpdatesAdd or LiveUpdatesRemove)
     * @param platform The emote platform  ("7TV", "BTTV", "FFZ")
     * @param actor The actor performing the action (possibly empty)
     * @param emoteName The updated emote's name
     * @return true, if the last message was replaced
     */
    bool tryReplaceLastLiveUpdateAddOrRemove(MessageFlag op,
                                             const QString &platform,
                                             const QString &actor,
                                             const QString &emoteName);

    // Data
    const QString subscriptionUrl_;
    const QString channelUrl_;
    const QString popoutPlayerUrl_;
    int chatterCount_{};
    UniqueAccess<StreamStatus> streamStatus_;
    UniqueAccess<RoomModes> roomModes;
    bool disconnected_{};
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        lastConnectedAt_{};
    std::atomic_flag loadingRecentMessages_ = ATOMIC_FLAG_INIT;
    std::unordered_map<QString, std::weak_ptr<MessageThread>> threads_;

protected:
    void messageRemovedFromStart(const MessagePtr &msg) override;

    Atomic<std::shared_ptr<const EmoteMap>> bttvEmotes_;
    Atomic<std::shared_ptr<const EmoteMap>> ffzEmotes_;
    Atomic<std::shared_ptr<const EmoteMap>> seventvEmotes_;
    Atomic<std::optional<EmotePtr>> ffzCustomModBadge_;
    Atomic<std::optional<EmotePtr>> ffzCustomVipBadge_;

    FfzChannelBadgeMap ffzChannelBadges_;
    ThreadGuard tgFfzChannelBadges_;

private:
    // Badges
    UniqueAccess<std::map<QString, std::map<QString, EmotePtr>>>
        badgeSets_;  // "subscribers": { "0": ... "3": ... "6": ...
    UniqueAccess<std::vector<CheerEmoteSet>> cheerEmoteSets_;
    UniqueAccess<std::map<QString, ChannelPointReward>> channelPointRewards_;
    boost::circular_buffer_space_optimized<QueuedRedemption>
        waitingRedemptions_{MAX_QUEUED_REDEMPTIONS};

    bool mod_ = false;
    bool vip_ = false;
    bool staff_ = false;
    UniqueAccess<QString> roomID_;

    // --
    QString lastSentMessage_;
    QObject lifetimeGuard_;
    QTimer chattersListTimer_;
    QTimer threadClearTimer_;
    QElapsedTimer titleRefreshedTimer_;
    QElapsedTimer clipCreationTimer_;
    bool isClipCreationInProgress{false};

    /**
     * This channels 7TV user-id,
     * empty if this channel isn't connected with 7TV.
     */
    QString seventvUserID_;
    /**
     * This channels current 7TV emote-set-id,
     * empty if this channel isn't connected with 7TV
     */
    QString seventvEmoteSetID_;
    /**
     * The index of the twitch connection in
     * 7TV's user representation.
     */
    size_t seventvUserTwitchConnectionIndex_{};

    /**
     * The next moment in time to signal activity in this channel to 7TV.
     * Or: Up until this moment we don't need to send activity.
     */
    QDateTime nextSeventvActivity_;

    /** The platform of the last live emote update ("7TV", "BTTV", "FFZ"). */
    QString lastLiveUpdateEmotePlatform_;
    /** The actor name of the last live emote update. */
    QString lastLiveUpdateEmoteActor_;
    /** A weak reference to the last live emote update message. */
    std::weak_ptr<const Message> lastLiveUpdateMessage_;
    /** A list of the emotes listed in the lat live emote update message. */
    std::vector<QString> lastLiveUpdateEmoteNames_;

    pajlada::Signals::SignalHolder signalHolder_;
    std::vector<boost::signals2::scoped_connection> bSignals_;

    friend class TwitchIrcServer;
    friend class TwitchMessageBuilder;
    friend class IrcMessageHandler;
    friend class Commands_E2E_Test;
};

}  // namespace chatterino
