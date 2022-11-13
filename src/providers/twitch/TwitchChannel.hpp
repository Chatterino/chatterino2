#pragma once

#include "Application.hpp"
#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "common/Channel.hpp"
#include "common/ChannelChatters.hpp"
#include "common/ChatterSet.hpp"
#include "common/Outcome.hpp"
#include "common/UniqueAccess.hpp"
#include "messages/MessageThread.hpp"
#include "providers/seventv/eventapi/SeventvEventAPIDispatch.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "util/QStringHash.hpp"

#include <QColor>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <pajlada/signals/signalholder.hpp>

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace chatterino {

// This is to make sure that combined emoji go through properly, see
// https://github.com/Chatterino/chatterino2/issues/3384 and
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
class SeventvEmotes;
class FfzEmotes;
class BttvEmotes;
class SeventvEmotes;

class TwitchIrcServer;

class TwitchChannel : public Channel, public ChannelChatters
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
        QString streamType;
    };

    struct RoomModes {
        bool submode = false;
        bool r9k = false;
        bool emoteOnly = false;
        int followerOnly = -1;
        int slowMode = 0;
    };

    explicit TwitchChannel(const QString &channelName);

    void initialize();

    // Channel methods
    virtual bool isEmpty() const override;
    virtual bool canSendMessage() const override;
    virtual void sendMessage(const QString &message) override;
    virtual void sendReply(const QString &message, const QString &replyId);
    virtual bool isMod() const override;
    bool isVip() const;
    bool isStaff() const;
    virtual bool isBroadcaster() const override;
    virtual bool hasHighRateLimit() const override;
    virtual bool canReconnect() const override;
    virtual void reconnect() override;
    void refreshTitle();
    void createClip();

    // Data
    const QString &subscriptionUrl();
    const QString &channelUrl();
    const QString &popoutPlayerUrl();
    int chatterCount();
    virtual bool isLive() const override;
    QString roomId() const;
    SharedAccessGuard<const RoomModes> accessRoomModes() const;
    SharedAccessGuard<const StreamStatus> accessStreamStatus() const;

    // Emotes
    boost::optional<EmotePtr> seventvEmote(const EmoteName &name) const;
    boost::optional<EmotePtr> bttvEmote(const EmoteName &name) const;
    boost::optional<EmotePtr> ffzEmote(const EmoteName &name) const;
    std::shared_ptr<const EmoteMap> bttvEmotes() const;
    std::shared_ptr<const EmoteMap> ffzEmotes() const;
    std::shared_ptr<const EmoteMap> seventvEmotes() const;

    virtual void refreshBTTVChannelEmotes(bool manualRefresh);
    virtual void refreshFFZChannelEmotes(bool manualRefresh);
    virtual void refreshSevenTVChannelEmotes(bool manualRefresh);

    const QString &seventvUserID() const;
    const QString &seventvEmoteSetID() const;

    /** Adds a 7TV channel emote to this channel. */
    void addSeventvEmote(const SeventvEventAPIEmoteAddDispatch &dispatch);
    /** Updates a 7TV channel emote's name in this channel */
    void updateSeventvEmote(const SeventvEventAPIEmoteUpdateDispatch &dispatch);
    /** Removes a 7TV channel emote from this channel */
    void removeSeventvEmote(const SeventvEventAPIEmoteRemoveDispatch &dispatch);
    /** Updates the current 7TV user. Currently, only the emote-set is updated. */
    void updateSeventvUser(
        const SeventvEventAPIUserConnectionUpdateDispatch &dispatch);

    // Update the channel's 7TV information (the channel's 7TV user ID and emote set ID)
    void updateSeventvData(const QString &newUserID,
                           const QString &newEmoteSetID);

    // Badges
    boost::optional<EmotePtr> ffzCustomModBadge() const;
    boost::optional<EmotePtr> ffzCustomVipBadge() const;
    boost::optional<EmotePtr> twitchBadge(const QString &set,
                                          const QString &version) const;

    // Cheers
    boost::optional<CheerEmote> cheerEmote(const QString &string);

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

    // Signals
    pajlada::Signals::NoArgSignal roomIdChanged;
    pajlada::Signals::NoArgSignal userStateChanged;
    pajlada::Signals::NoArgSignal liveStatusChanged;
    pajlada::Signals::NoArgSignal roomModesChanged;

    // Channel point rewards
    pajlada::Signals::SelfDisconnectingSignal<ChannelPointReward>
        channelPointRewardAdded;
    void addChannelPointReward(const ChannelPointReward &reward);
    bool isChannelPointRewardKnown(const QString &rewardId);
    boost::optional<ChannelPointReward> channelPointReward(
        const QString &rewardId) const;

private:
    struct NameOptions {
        QString displayName;
        QString localizedName;
    } nameOptions;

private:
    // Methods
    void refreshLiveStatus();
    void parseLiveStatus(bool live, const HelixStream &stream);
    void refreshPubSub();
    void refreshChatters();
    void refreshBadges();
    void refreshCheerEmotes();
    void loadRecentMessages();
    void loadRecentMessagesReconnect();
    void fetchDisplayName();
    void cleanUpReplyThreads();
    void showLoginMessage();

    void setLive(bool newLiveStatus);
    void setMod(bool value);
    void setVIP(bool value);
    void setStaff(bool value);
    void setRoomId(const QString &id);
    void setRoomModes(const RoomModes &roomModes_);
    void setDisplayName(const QString &name);
    void setLocalizedName(const QString &name);

    const QString &getDisplayName() const override;
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
    int chatterCount_;
    UniqueAccess<StreamStatus> streamStatus_;
    UniqueAccess<RoomModes> roomModes_;
    std::atomic_flag loadingRecentMessages_ = ATOMIC_FLAG_INIT;
    std::unordered_map<QString, std::weak_ptr<MessageThread>> threads_;

protected:
    Atomic<std::shared_ptr<const EmoteMap>> bttvEmotes_;
    Atomic<std::shared_ptr<const EmoteMap>> ffzEmotes_;
    Atomic<std::shared_ptr<const EmoteMap>> seventvEmotes_;
    Atomic<boost::optional<EmotePtr>> ffzCustomModBadge_;
    Atomic<boost::optional<EmotePtr>> ffzCustomVipBadge_;

private:
    // Badges
    UniqueAccess<std::map<QString, std::map<QString, EmotePtr>>>
        badgeSets_;  // "subscribers": { "0": ... "3": ... "6": ...
    UniqueAccess<std::vector<CheerEmoteSet>> cheerEmoteSets_;
    UniqueAccess<std::map<QString, ChannelPointReward>> channelPointRewards_;

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
    size_t seventvUserTwitchConnectionIndex_;

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
};

}  // namespace chatterino
