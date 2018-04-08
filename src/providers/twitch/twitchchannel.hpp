#pragma once

#include <IrcConnection>

#include "channel.hpp"
#include "common.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"
#include "util/concurrentmap.hpp"

#include <mutex>

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchServer;

class TwitchChannel final : public Channel
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

    QString roomID;

    StreamStatus GetStreamStatus() const
    {
        std::lock_guard<std::mutex> lock(this->streamStatusMutex);
        return this->streamStatus;
    }

    struct NameOptions {
        QString displayName;
        QString localizedName;
    };

    bool IsLive() const
    {
        std::lock_guard<std::mutex> lock(this->streamStatusMutex);
        return this->streamStatus.live;
    }

private:
    explicit TwitchChannel(const QString &channelName, Communi::IrcConnection *readConnection);

    void setLive(bool newLiveStatus);
    void refreshLiveStatus();

    mutable std::mutex streamStatusMutex;
    StreamStatus streamStatus;

    void fetchRecentMessages();

    bool mod;
    QByteArray messageSuffix;
    QString lastSentMessage;

    Communi::IrcConnection *readConnection;

    friend class TwitchServer;

    // Key = login name
    std::map<QString, NameOptions> recentChatters;
    std::mutex recentChattersMutex;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
