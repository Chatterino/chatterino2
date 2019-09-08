#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "common/Channel.hpp"
#include "common/Outcome.hpp"
#include "common/UniqueAccess.hpp"
#include "common/UsernameSet.hpp"
#include "providers/twitch/TwitchEmotes.hpp"

#include <rapidjson/document.h>
#include <IrcConnection>
#include <QColor>
#include <QRegularExpression>
#include <boost/optional.hpp>
#include <mutex>
#include <pajlada/signals/signalholder.hpp>
#include <unordered_map>

namespace chatterino {

enum class HighlightState;

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;

class TwitchBadges;
class FfzEmotes;
class BttvEmotes;

class TwitchServer;

class TwitchChannel : public Channel, pajlada::Signals::SignalHolder
{
public:
    struct StreamStatus {
        bool live = false;
        bool rerun = false;
        unsigned viewerCount = 0;
        QString title;
        QString game;
        QString uptime;
        QString streamType;
    };

    struct RoomModes {
        bool submode = false;
        bool r9k = false;
        bool emoteOnly = false;
        int followerOnly = -1;
        int slowMode = 0;
        QString broadcasterLang;
    };

    void initialize();

    // Channel methods
    virtual bool isEmpty() const override;
    virtual bool canSendMessage() const override;
    virtual void sendMessage(const QString &message) override;
    virtual bool isMod() const override;
    bool isVIP() const;
    bool isStaff() const;
    virtual bool isBroadcaster() const override;
    virtual bool hasHighRateLimit() const override;

    // Data
    const QString &subscriptionUrl();
    const QString &channelUrl();
    const QString &popoutPlayerUrl();
    virtual bool isLive() const override;
    QString roomId() const;
    AccessGuard<const RoomModes> accessRoomModes() const;
    AccessGuard<const StreamStatus> accessStreamStatus() const;
    AccessGuard<const UsernameSet> accessChatters() const;

    // Emotes
    const TwitchBadges &globalTwitchBadges() const;
    const BttvEmotes &globalBttv() const;
    const FfzEmotes &globalFfz() const;
    boost::optional<EmotePtr> bttvEmote(const EmoteName &name) const;
    boost::optional<EmotePtr> ffzEmote(const EmoteName &name) const;
    std::shared_ptr<const EmoteMap> bttvEmotes() const;
    std::shared_ptr<const EmoteMap> ffzEmotes() const;

    virtual void refreshBTTVChannelEmotes();
    virtual void refreshFFZChannelEmotes();

    // Badges
    boost::optional<EmotePtr> ffzCustomModBadge() const;
    boost::optional<EmotePtr> twitchBadge(const QString &set,
                                          const QString &version) const;

    // Cheers
    boost::optional<CheerEmote> cheerEmote(const QString &string);

    // Signals
    pajlada::Signals::NoArgSignal roomIdChanged;
    pajlada::Signals::NoArgSignal userStateChanged;
    pajlada::Signals::NoArgSignal liveStatusChanged;
    pajlada::Signals::NoArgSignal roomModesChanged;

protected:
    void addRecentChatter(const MessagePtr &message) override;

private:
    struct NameOptions {
        QString displayName;
        QString localizedName;
    };

protected:
    explicit TwitchChannel(const QString &channelName,
                           TwitchBadges &globalTwitchBadges,
                           BttvEmotes &globalBttv, FfzEmotes &globalFfz);

private:
    // Methods
    void refreshLiveStatus();
    Outcome parseLiveStatus(const rapidjson::Document &document);
    void refreshPubsub();
    void refreshChatters();
    void refreshBadges();
    void refreshCheerEmotes();
    void loadRecentMessages();

    void addJoinedUser(const QString &user);
    void addPartedUser(const QString &user);
    void setLive(bool newLiveStatus);
    void setMod(bool value);
    void setVIP(bool value);
    void setStaff(bool value);
    void setRoomId(const QString &id);
    void setRoomModes(const RoomModes &roomModes_);

    // Data
    const QString subscriptionUrl_;
    const QString channelUrl_;
    const QString popoutPlayerUrl_;
    UniqueAccess<StreamStatus> streamStatus_;
    UniqueAccess<RoomModes> roomModes_;
    UniqueAccess<UsernameSet> chatters_;  // maps 2 char prefix to set of names

    // Emotes
    TwitchBadges &globalTwitchBadges_;

protected:
    BttvEmotes &globalBttv_;
    FfzEmotes &globalFfz_;
    Atomic<std::shared_ptr<const EmoteMap>> bttvEmotes_;
    Atomic<std::shared_ptr<const EmoteMap>> ffzEmotes_;
    Atomic<boost::optional<EmotePtr>> ffzCustomModBadge_;

private:
    // Badges
    UniqueAccess<std::map<QString, std::map<QString, EmotePtr>>>
        badgeSets_;  // "subscribers": { "0": ... "3": ... "6": ...
    UniqueAccess<std::vector<CheerEmoteSet>> cheerEmoteSets_;

    bool mod_ = false;
    bool vip_ = false;
    bool staff_ = false;
    UniqueAccess<QString> roomID_;

    UniqueAccess<QStringList> joinedUsers_;
    bool joinedUsersMergeQueued_ = false;
    UniqueAccess<QStringList> partedUsers_;
    bool partedUsersMergeQueued_ = false;

    // --
    QString lastSentMessage_;
    QObject lifetimeGuard_;
    QTimer liveStatusTimer_;
    QTimer chattersListTimer_;

    friend class TwitchServer;
    friend class TwitchMessageBuilder;
    friend class IrcMessageHandler;
};

}  // namespace chatterino
