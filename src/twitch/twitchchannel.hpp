#pragma once

#include "channel.hpp"
#include "concurrentmap.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"

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

    const std::shared_ptr<chatterino::EmoteMap> bttvChannelEmotes;
    const std::shared_ptr<chatterino::EmoteMap> ffzChannelEmotes;

    const QString subscriptionURL;
    const QString channelURL;
    const QString popoutPlayerURL;

    void setRoomID(const QString &_roomID);
    boost::signals2::signal<void()> roomIDchanged;
    boost::signals2::signal<void()> onlineStatusChanged;

    pajlada::Signals::NoArgBoltSignal fetchMessages;

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
};

}  // namespace twitch
}  // namespace chatterino
