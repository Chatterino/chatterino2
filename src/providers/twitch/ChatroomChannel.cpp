#include "ChatroomChannel.hpp"

#include <QDebug>
#include "TwitchApi.hpp"
#include "common/Common.hpp"
#include "messages/Emote.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/bttv/LoadBttvChannelEmote.hpp"
#include "singletons/Emotes.hpp"

namespace chatterino {

ChatroomChannel::ChatroomChannel(const QString &channelName,
                                 TwitchBadges &globalTwitchBadges,
                                 BttvEmotes &globalBttv, FfzEmotes &globalFfz)
    : TwitchChannel(channelName, globalTwitchBadges, globalBttv, globalFfz)
{
    auto listRef = channelName.splitRef(":");
    if (listRef.size() > 2)
    {
        this->chatroomOwnerId = listRef[1].toString();
    }
}

void ChatroomChannel::refreshBTTVChannelEmotes()
{
    if (this->chatroomOwnerId.isEmpty())
    {
        return;
    }
    TwitchApi::findUserName(
        this->chatroomOwnerId,
        [this, weak = weakOf<Channel>(this)](QString username) {
            BttvEmotes::loadChannel(username, [this, weak](auto &&emoteMap) {
                if (auto shared = weak.lock())
                    this->bttvEmotes_.set(
                        std::make_shared<EmoteMap>(std::move(emoteMap)));
            });
            if (auto shared = weak.lock())
            {
                this->chatroomOwnerName = username;
            }
        });
}
void ChatroomChannel::refreshFFZChannelEmotes()
{
    if (this->chatroomOwnerId.isEmpty())
    {
        return;
    }
    FfzEmotes::loadChannel(
        this->chatroomOwnerId,
        [this](auto &&emoteMap) {
            this->ffzEmotes_.set(
                std::make_shared<EmoteMap>(std::move(emoteMap)));
        },
        [this](auto &&modBadge) {
            this->ffzCustomModBadge_.set(std::move(modBadge));
        });
}

const QString &ChatroomChannel::getDisplayName() const
{
    return this->chatroomOwnerName;
}

}  // namespace chatterino
