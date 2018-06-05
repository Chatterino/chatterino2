#pragma once

#include <IrcConnection>

#include "channel.hpp"
#include "common.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"
#include "util/concurrentmap.hpp"
#include "util/mutexvalue.hpp"

#include <pajlada/signals/signalholder.hpp>

#include <mutex>

namespace chatterino {
namespace providers {
namespace twitch {

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

    bool isMod() const override;
    void setMod(bool value);
    bool isBroadcaster();
    bool hasModRights();

    void addRecentChatter(const std::shared_ptr<messages::Message> &message) final;
    void addJoinedUser(const QString &user);
    void addPartedUser(const QString &user);

    const std::shared_ptr<chatterino::util::EmoteMap> bttvChannelEmotes;
    const std::shared_ptr<chatterino::util::EmoteMap> ffzChannelEmotes;

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
    void setRoomModes(const RoomModes &roomModes);

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

    mutable std::mutex streamStatusMutex;
    StreamStatus streamStatus;

    mutable std::mutex userStateMutex;
    UserState userState;

    void fetchRecentMessages();

    bool mod;
    QByteArray messageSuffix;
    QString lastSentMessage;
    RoomModes roomModes;
    std::mutex roomModeMutex;

    QObject object;
    std::mutex joinedUserMutex;
    QStringList joinedUsers;
    bool joinedUsersMergeQueued = false;
    std::mutex partedUserMutex;
    QStringList partedUsers;
    bool partedUsersMergeQueued = false;

    Communi::IrcConnection *readConnection;

    friend class TwitchServer;

    // Key = login name
    std::map<QString, NameOptions> recentChatters;
    std::mutex recentChattersMutex;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
