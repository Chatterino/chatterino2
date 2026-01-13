// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/commands/builtin/twitch/SendReply.hpp"

#include "controllers/commands/CommandContext.hpp"
#include "messages/Message.hpp"
#include "messages/MessageThread.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

#include <ranges>

namespace chatterino::commands {

QString sendReply(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /reply command only works in Twitch channels.");
        return "";
    }

    if (ctx.words.size() < 3)
    {
        ctx.channel->addSystemMessage("Usage: /reply <username> <message>");
        return "";
    }

    QString username = ctx.words[1];
    stripChannelName(username);

    auto snapshot = ctx.twitchChannel->getMessageSnapshot();
    for (const auto &msg : snapshot | std::views::reverse)
    {
        if (msg->loginName.compare(username, Qt::CaseInsensitive) == 0)
        {
            // found most recent message by user
            if (msg->replyThread == nullptr)
            {
                // prepare thread if one does not exist
                auto thread = std::make_shared<MessageThread>(msg);
                ctx.twitchChannel->addReplyThread(thread);
            }

            QString reply = ctx.words.mid(2).join(" ");
            ctx.twitchChannel->sendReply(reply, msg->id);
            return "";
        }
    }

    ctx.channel->addSystemMessage("A message from that user wasn't found.");

    return "";
}

}  // namespace chatterino::commands
