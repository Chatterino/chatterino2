#pragma once

#include "channel.hpp"
#include "concurrentmap.hpp"
#include "ircmanager.hpp"

namespace chatterino {
namespace twitch {

class TwitchChannel : public Channel
{
public:
    explicit TwitchChannel(EmoteManager &emoteManager, IrcManager &ircManager,
                           const QString &channelName, bool isSpecial = false);

    void reloadChannelEmotes();

    bool isEmpty() const override;
    bool canSendMessage() const override;
    void sendMessage(const QString &message) override;

    const QString &getSubLink() const;
    const QString &getChannelLink() const;
    const QString &getPopoutPlayerLink() const;

    void setRoomID(std::string id);
    boost::signals2::signal<void()> roomIDchanged;

    std::string roomID;
    bool isLive;
    QString streamViewerCount;
    QString streamStatus;
    QString streamGame;
    QString streamUptime;

    const std::shared_ptr<EmoteMap> bttvChannelEmotes;
    const std::shared_ptr<EmoteMap> ffzChannelEmotes;

private:
    EmoteManager &emoteManager;
    IrcManager &ircManager;

    QString subLink;
    QString channelLink;
    QString popoutPlayerLink;
    bool isSpecial;
};
}
}
