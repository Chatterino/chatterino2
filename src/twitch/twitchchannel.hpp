#pragma once

#include "channel.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"
#include "util/concurrentmap.hpp"

namespace chatterino {
namespace twitch {

class TwitchChannel : public Channel
{
    QTimer *liveStatusTimer;

public:
    explicit TwitchChannel(const QString &channelName);
    ~TwitchChannel();

    void reloadChannelEmotes();

    bool isEmpty() const override;
    bool canSendMessage() const override;
    void sendMessage(const QString &message) override;

    bool isMod() const override;
    void setMod(bool value);
    bool isBroadcaster();
    bool hasModRights();

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

private:
    void setLive(bool newLiveStatus);
    void refreshLiveStatus();

    void fetchRecentMessages();

    boost::signals2::connection connectedConnection;

    bool mod;
};

}  // namespace twitch
}  // namespace chatterino
