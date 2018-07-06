#pragma once

#include <IrcConnection>

#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "common/MutexValue.hpp"
#include "singletons/Emotes.hpp"
#include "util/ConcurrentMap.hpp"

#include <pajlada/signals/signalholder.hpp>

#include <mutex>

namespace chatterino {

class TwitchServer;

class TwitchChannel final : public Channel, pajlada::Signals::SignalHolder
{
    QTimer *liveStatusTimer;
    QTimer *chattersListTimer;

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

    ~TwitchChannel() final;

    void reloadChannelEmotes();

    bool isEmpty() const override;
    bool canSendMessage() const override;
    void sendMessage(const QString &message) override;

    virtual bool isMod() const override;
    void setMod(bool value);
    virtual bool isBroadcaster() const override;

    void addRecentChatter(const std::shared_ptr<Message> &message) final;
    void addJoinedUser(const QString &user);
    void addPartedUser(const QString &user);

    const std::shared_ptr<EmoteMap> bttvChannelEmotes;
    const std::shared_ptr<EmoteMap> ffzChannelEmotes;

    const QString subscriptionURL;
    const QString channelURL;
    const QString popoutPlayerURL;

    void setRoomID(const QString &_roomID);
    pajlada::Signals::NoArgSignal roomIDchanged;
    pajlada::Signals::NoArgSignal updateLiveInfo;

    pajlada::Signals::NoArgBoltSignal fetchMessages;
    pajlada::Signals::NoArgSignal userStateChanged;
    pajlada::Signals::NoArgSignal roomModesChanged;

    QString roomID;

    RoomModes getRoomModes();
    void setRoomModes(const RoomModes &roomModes_);

    StreamStatus getStreamStatus() const;

    struct NameOptions {
        QString displayName;
        QString localizedName;
    };

    bool isLive() const;

private:
    explicit TwitchChannel(const QString &channelName, Communi::IrcConnection *readConnection);

    void setLive(bool newLiveStatus);
    void refreshLiveStatus();
    void startRefreshLiveStatusTimer(int intervalMS);
    void fetchRecentMessages();

    mutable std::mutex streamStatusMutex_;
    StreamStatus streamStatus_;

    mutable std::mutex userStateMutex_;
    UserState userState_;

    bool mod_ = false;
    QByteArray messageSuffix_;
    QString lastSentMessage_;
    RoomModes roomModes_;
    std::mutex roomModeMutex_;

    QObject object_;
    std::mutex joinedUserMutex_;
    QStringList joinedUsers_;
    bool joinedUsersMergeQueued_ = false;
    std::mutex partedUserMutex_;
    QStringList partedUsers_;
    bool partedUsersMergeQueued_ = false;

    Communi::IrcConnection *readConnection_ = nullptr;

    // Key = login name
    std::map<QString, NameOptions> recentChatters_;
    std::mutex recentChattersMutex_;

    friend class TwitchServer;
};

}  // namespace chatterino
