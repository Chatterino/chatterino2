#include "controllers/commands/builtin/twitch/UpdateChannel.hpp"

#include "common/Channel.hpp"
#include "common/NetworkResult.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino::commands {

QString setTitle(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(
            makeSystemMessage("Usage: /settitle <stream title>"));
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(
            makeSystemMessage("Unable to set title of non-Twitch channel."));
        return "";
    }

    auto status = ctx.twitchChannel->accessStreamStatus();
    auto title = ctx.words.mid(1).join(" ");
    getHelix()->updateChannel(
        ctx.twitchChannel->roomId(), "", "", title,
        [channel{ctx.channel}, title](const auto &result) {
            (void)result;

            channel->addMessage(
                makeSystemMessage(QString("Updated title to %1").arg(title)));
        },
        [channel{ctx.channel}] {
            channel->addMessage(
                makeSystemMessage("Title update failed! Are you "
                                  "missing the required scope?"));
        });

    return "";
}

}  // namespace chatterino::commands
