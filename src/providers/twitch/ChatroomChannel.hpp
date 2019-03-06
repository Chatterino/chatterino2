#pragma once

#include "TwitchChannel.hpp"

#include <QString>
#include <atomic>

namespace chatterino
{
    class ChatroomChannel : public TwitchChannel
    {
    protected:
        explicit ChatroomChannel(const QString& channelName,
            TwitchBadges& globalTwitchBadges, BttvEmotes& globalBttv,
            FfzEmotes& globalFfz);
        virtual void refreshChannelEmotes() override;
        virtual const QString& getDisplayName() const override;

        QString chatroomOwnerId;
        QString chatroomOwnerName;

        friend class TwitchServer;
        friend class TwitchMessageBuilder;
        friend class IrcMessageHandler;
    };

}  // namespace chatterino
