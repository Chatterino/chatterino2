#include "controllers/commands/builtin/Misc.hpp"

#include "common/Channel.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"

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

}  // namespace chatterino::commands
