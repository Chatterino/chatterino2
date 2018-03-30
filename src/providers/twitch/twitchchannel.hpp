#pragma once

#include <IrcConnection>

#include "channel.hpp"
#include "common.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"
#include "util/concurrentmap.hpp"

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchServer;

class TwitchChannel final : public Channel
{
    QTimer *liveStatusTimer;
    QTimer *chattersListTimer;

public:
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
    boost::signals2::signal<void()> roomIDchanged;
    boost::signals2::signal<void()> onlineStatusChanged;

    pajlada::Signals::NoArgBoltSignal fetchMessages;
    boost::signals2::signal<void()> userStateChanged;

    QString roomID;
    bool isLive;
    QString streamViewerCount;
    QString streamStatus;
    QString streamGame;
    QString streamUptime;

    struct NameOptions {
        QString displayName;
        QString localizedName;
    };

private:
    explicit TwitchChannel(const QString &channelName, Communi::IrcConnection *readConnection);

    void setLive(bool newLiveStatus);
    void refreshLiveStatus();

    void fetchRecentMessages();

    boost::signals2::connection connectedConnection;

    bool mod;
    QByteArray messageSuffix;
    QString lastSentMessage;

    Communi::IrcConnection *readConnecetion;

    friend class TwitchServer;

    // Key = login name
    std::map<QString, NameOptions> recentChatters;
    std::mutex recentChattersMutex;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
