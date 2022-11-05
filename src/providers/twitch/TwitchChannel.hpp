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
    boost::optional<EmotePtr> bttvEmote(const EmoteName &name) const;
    boost::optional<EmotePtr> ffzEmote(const EmoteName &name) const;
    boost::optional<EmotePtr> seventvEmote(const EmoteName &name) const;
    std::shared_ptr<const EmoteMap> bttvEmotes() const;
    std::shared_ptr<const EmoteMap> ffzEmotes() const;
    std::shared_ptr<const EmoteMap> seventvEmotes() const;

    virtual void refreshBTTVChannelEmotes(bool manualRefresh);
    virtual void refreshFFZChannelEmotes(bool manualRefresh);
    virtual void refreshSevenTVChannelEmotes(bool manualRefresh);

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

    pajlada::Signals::SignalHolder signalHolder_;
    std::vector<boost::signals2::scoped_connection> bSignals_;

    friend class TwitchIrcServer;
    friend class TwitchMessageBuilder;
    friend class IrcMessageHandler;
};

}  // namespace chatterino
