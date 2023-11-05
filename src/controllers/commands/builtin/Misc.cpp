#include "controllers/commands/builtin/Misc.hpp"

#include "common/Channel.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

#include <QDesktopServices>
#include <QString>
#include <QUrl>

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

QString user(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(
            makeSystemMessage("Usage: /user <user> [channel]"));
        return "";
    }
    QString userName = ctx.words[1];
    stripUserName(userName);

    QString channelName = ctx.channel->getName();

    if (ctx.words.size() > 2)
    {
        channelName = ctx.words[2];
        stripChannelName(channelName);
    }
    openTwitchUsercard(channelName, userName);

    return "";
}

QString requests(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    QString target(ctx.words.value(1));

    if (target.isEmpty())
    {
        if (ctx.channel->getType() == Channel::Type::Twitch &&
            !ctx.channel->isEmpty())
        {
            target = ctx.channel->getName();
        }
        else
        {
            ctx.channel->addMessage(makeSystemMessage(
                "Usage: /requests [channel]. You can also use the command "
                "without arguments in any Twitch channel to open its "
                "channel points requests queue. Only the broadcaster and "
                "moderators have permission to view the queue."));
            return "";
        }
    }

    stripChannelName(target);
    QDesktopServices::openUrl(QUrl(
        QString("https://www.twitch.tv/popout/%1/reward-queue").arg(target)));

    return "";
}

QString lowtrust(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    QString target(ctx.words.value(1));

    if (target.isEmpty())
    {
        if (ctx.channel->getType() == Channel::Type::Twitch &&
            !ctx.channel->isEmpty())
        {
            target = ctx.channel->getName();
        }
        else
        {
            ctx.channel->addMessage(makeSystemMessage(
                "Usage: /lowtrust [channel]. You can also use the command "
                "without arguments in any Twitch channel to open its "
                "suspicious user activity feed. Only the broadcaster and "
                "moderators have permission to view this feed."));
            return "";
        }
    }

    stripChannelName(target);
    QDesktopServices::openUrl(QUrl(
        QString("https://www.twitch.tv/popout/moderator/%1/low-trust-users")
            .arg(target)));

    return "";
}

}  // namespace chatterino::commands
