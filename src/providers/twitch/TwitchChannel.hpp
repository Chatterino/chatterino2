#pragma once

#include <IrcConnection>

#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "common/MutexValue.hpp"
#include "common/UniqueAccess.hpp"
#include "singletons/Emotes.hpp"
#include "util/ConcurrentMap.hpp"

#include <pajlada/signals/signalholder.hpp>

#include <mutex>

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

    const EmoteMap &getFfzEmotes() const;
    const EmoteMap &getBttvEmotes() const;
    const QString &getSubscriptionUrl();
    const QString &getChannelUrl();
    const QString &getPopoutPlayerUrl();

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

    explicit TwitchChannel(const QString &channelName, Communi::IrcConnection *readConnection);

    // Methods
    void refreshLiveStatus();
    bool parseLiveStatus(const rapidjson::Document &document);
    void refreshPubsub();
    void refreshViewerList();
    bool parseViewerList(const QJsonObject &jsonRoot);
    void loadRecentMessages();
    bool parseRecentMessages(const QJsonObject &jsonRoot);

    void setLive(bool newLiveStatus);

    // Twitch data
    UniqueAccess<StreamStatus> streamStatus_;
    UniqueAccess<UserState> userState_;
    UniqueAccess<RoomModes> roomModes_;

    const std::shared_ptr<EmoteMap> bttvEmotes_;
    const std::shared_ptr<EmoteMap> ffzEmotes_;
    const QString subscriptionUrl_;
    const QString channelUrl_;
    const QString popoutPlayerUrl_;

    bool mod_ = false;
    MutexValue<QString> roomID_;

    UniqueAccess<QStringList> joinedUsers_;
    bool joinedUsersMergeQueued_ = false;
    UniqueAccess<QStringList> partedUsers_;
    bool partedUsersMergeQueued_ = false;

    // --
    QByteArray messageSuffix_;
    QString lastSentMessage_;
    QObject lifetimeGuard_;
    QTimer liveStatusTimer_;
    QTimer chattersListTimer_;
    Communi::IrcConnection *readConnection_ = nullptr;

    friend class TwitchServer;
};

}  // namespace chatterino
