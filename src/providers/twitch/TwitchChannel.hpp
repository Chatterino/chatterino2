#pragma once

#include <IrcConnection>

#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "common/MutexValue.hpp"
#include "common/UniqueAccess.hpp"
#include "messages/Emote.hpp"
#include "singletons/Emotes.hpp"
#include "util/ConcurrentMap.hpp"

#include <pajlada/signals/signalholder.hpp>

#include <mutex>
#include <unordered_map>

namespace chatterino {

class TwitchServer;

class TwitchChannel final : public Channel, pajlada::Signals::SignalHolder
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

    struct UserState {
        bool mod;
        bool broadcaster;
    };

    struct RoomModes {
        bool submode = false;
        bool r9k = false;
        bool emoteOnly = false;
        //        int folowerOnly = 0;
        int slowMode = 0;
        QString broadcasterLang;
    };

    void refreshChannelEmotes();

    // Channel methods
    virtual bool isEmpty() const override;
    virtual bool canSendMessage() const override;
    virtual void sendMessage(const QString &message) override;

    // Auto completion
    void addRecentChatter(const std::shared_ptr<Message> &message) final;
    void addJoinedUser(const QString &user);
    void addPartedUser(const QString &user);

    // Twitch data
    bool isLive() const;
    virtual bool isMod() const override;
    void setMod(bool value);
    virtual bool isBroadcaster() const override;

    QString getRoomId() const;
    void setRoomId(const QString &id);
    const AccessGuard<RoomModes> accessRoomModes() const;
    void setRoomModes(const RoomModes &roomModes_);
    const AccessGuard<StreamStatus> accessStreamStatus() const;

    boost::optional<EmotePtr> getBttvEmote(const EmoteName &name) const;
    boost::optional<EmotePtr> getFfzEmote(const EmoteName &name) const;
    AccessGuard<const EmoteMap> accessBttvEmotes() const;
    AccessGuard<const EmoteMap> accessFfzEmotes() const;
    const QString &getSubscriptionUrl();
    const QString &getChannelUrl();
    const QString &getPopoutPlayerUrl();

    boost::optional<EmotePtr> getTwitchBadge(const QString &set, const QString &version) const;

    // Signals
    pajlada::Signals::NoArgSignal roomIdChanged;
    pajlada::Signals::NoArgSignal liveStatusChanged;
    pajlada::Signals::NoArgSignal userStateChanged;
    pajlada::Signals::NoArgSignal roomModesChanged;

private:
    struct NameOptions {
        QString displayName;
        QString localizedName;
    };

    struct CheerEmote {
        // a Cheermote indicates one tier
        QColor color;
        int minBits;

        EmotePtr animatedEmote;
        EmotePtr staticEmote;
    };

    struct CheerEmoteSet {
        QRegularExpression regex;
        std::vector<CheerEmote> cheerEmotes;
    };

    explicit TwitchChannel(const QString &channelName);

    // Methods
    void refreshLiveStatus();
    Outcome parseLiveStatus(const rapidjson::Document &document);
    void refreshPubsub();
    void refreshViewerList();
    Outcome parseViewerList(const QJsonObject &jsonRoot);
    void loadRecentMessages();
    Outcome parseRecentMessages(const QJsonObject &jsonRoot);

    void setLive(bool newLiveStatus);

    void loadBadges();
    void loadCheerEmotes();

    // Twitch data
    UniqueAccess<StreamStatus> streamStatus_;
    UniqueAccess<UserState> userState_;
    UniqueAccess<RoomModes> roomModes_;

    UniqueAccess<EmoteMap> bttvEmotes_;
    UniqueAccess<EmoteMap> ffzEmotes_;
    const QString subscriptionUrl_;
    const QString channelUrl_;
    const QString popoutPlayerUrl_;

    bool mod_ = false;
    MutexValue<QString> roomID_;

    UniqueAccess<QStringList> joinedUsers_;
    bool joinedUsersMergeQueued_ = false;
    UniqueAccess<QStringList> partedUsers_;
    bool partedUsersMergeQueued_ = false;

    // "subscribers": { "0": ... "3": ... "6": ...
    UniqueAccess<std::map<QString, std::map<QString, EmotePtr>>> badgeSets_;
    std::vector<CheerEmoteSet> cheerEmoteSets_;

    // --
    QByteArray messageSuffix_;
    QString lastSentMessage_;
    QObject lifetimeGuard_;
    QTimer liveStatusTimer_;
    QTimer chattersListTimer_;

    friend class TwitchServer;
};

}  // namespace chatterino
