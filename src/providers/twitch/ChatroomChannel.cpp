#include "ChatroomChannel.hpp"

#include <QDebug>
#include "TwitchApi.hpp"
#include "common/Common.hpp"
#include "messages\Emote.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/bttv/LoadBttvChannelEmote.hpp"
#include "singletons/Emotes.hpp"

namespace chatterino {

ChatroomChannel::ChatroomChannel(const QString &channelName,
                                 TwitchBadges &globalTwitchBadges,
                                 BttvEmotes &globalBttv, FfzEmotes &globalFfz)
    : TwitchChannel(channelName, globalTwitchBadges, globalBttv, globalFfz)
{
    auto list = channelName.split(":");
    if (list.size() > 2)
    {
        this->chatroomOwnerId = list[1];
    }
}

void ChatroomChannel::refreshChannelEmotes()
{
    TwitchApi::findUserName(
        this->chatroomOwnerId,
        [this, weak = weakOf<Channel>(this)](QString username) {
            qDebug() << username;
            BttvEmotes::loadChannel(username, [this, weak](auto &&emoteMap) {
                if (auto shared = weak.lock())
                    this->bttvEmotes_.set(
                        std::make_shared<EmoteMap>(std::move(emoteMap)));
            });
            FfzEmotes::loadChannel(username, [this, weak](auto &&emoteMap) {
                if (auto shared = weak.lock())
                    this->ffzEmotes_.set(
                        std::make_shared<EmoteMap>(std::move(emoteMap)));
            });
        });
}

}  // namespace chatterino
