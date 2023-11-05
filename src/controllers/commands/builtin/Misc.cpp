#include "controllers/commands/builtin/Misc.hpp"

#include "common/Channel.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QString>

namespace chatterino::commands {

QString follow(const CommandContext &ctx)
{
    if (ctx.twitchChannel == nullptr)
    {
        return "";
    }
    ctx.channel->addMessage(makeSystemMessage(
        "Twitch has removed the ability to follow users through "
        "third-party applications. For more information, see "
        "https://github.com/Chatterino/chatterino2/issues/3076"));
    return "";
}

QString unfollow(const CommandContext &ctx)
{
    if (ctx.twitchChannel == nullptr)
    {
        return "";
    }
    ctx.channel->addMessage(makeSystemMessage(
        "Twitch has removed the ability to unfollow users through "
        "third-party applications. For more information, see "
        "https://github.com/Chatterino/chatterino2/issues/3076"));
    return "";
}

QString uptime(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "The /uptime command only works in Twitch Channels"));
        return "";
    }

    const auto &streamStatus = ctx.twitchChannel->accessStreamStatus();

    QString messageText =
        streamStatus->live ? streamStatus->uptime : "Channel is not live.";

    ctx.channel->addMessage(makeSystemMessage(messageText));

    return "";
}

}  // namespace chatterino::commands
