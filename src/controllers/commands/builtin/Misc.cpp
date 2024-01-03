#include "controllers/commands/builtin/Misc.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clipboard.hpp"
#include "util/FormatTime.hpp"
#include "util/IncognitoBrowser.hpp"
#include "util/StreamLink.hpp"
#include "util/Twitch.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/Window.hpp"

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
            "The /uptime command only works in Twitch Channels."));
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

QString clip(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (const auto type = ctx.channel->getType();
        type != Channel::Type::Twitch && type != Channel::Type::TwitchWatching)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "The /clip command only works in Twitch Channels."));
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "The /clip command only works in Twitch Channels."));
        return "";
    }

    ctx.twitchChannel->createClip();

    return "";
}

QString marker(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "The /marker command only works in Twitch channels."));
        return "";
    }

    // Avoid Helix calls without Client ID and/or OAuth Token
    if (getApp()->accounts->twitch.getCurrent()->isAnon())
    {
        ctx.channel->addMessage(makeSystemMessage(
            "You need to be logged in to create stream markers!"));
        return "";
    }

    // Exact same message as in webchat
    if (!ctx.twitchChannel->isLive())
    {
        ctx.channel->addMessage(makeSystemMessage(
            "You can only add stream markers during live streams. Try "
            "again when the channel is live streaming."));
        return "";
    }

    auto arguments = ctx.words;
    arguments.removeFirst();

    getHelix()->createStreamMarker(
        // Limit for description is 140 characters, webchat just crops description
        // if it's >140 characters, so we're doing the same thing
        ctx.twitchChannel->roomId(), arguments.join(" ").left(140),
        [channel{ctx.channel},
         arguments](const HelixStreamMarker &streamMarker) {
            channel->addMessage(makeSystemMessage(
                QString("Successfully added a stream marker at %1%2")
                    .arg(formatTime(streamMarker.positionSeconds))
                    .arg(streamMarker.description.isEmpty()
                             ? ""
                             : QString(": \"%1\"")
                                   .arg(streamMarker.description))));
        },
        [channel{ctx.channel}](auto error) {
            QString errorMessage("Failed to create stream marker - ");

            switch (error)
            {
                case HelixStreamMarkerError::UserNotAuthorized: {
                    errorMessage +=
                        "you don't have permission to perform that action.";
                }
                break;

                case HelixStreamMarkerError::UserNotAuthenticated: {
                    errorMessage += "you need to re-authenticate.";
                }
                break;

                // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                case HelixStreamMarkerError::Unknown:
                default: {
                    errorMessage += "an unknown error occurred.";
                }
                break;
            }

            channel->addMessage(makeSystemMessage(errorMessage));
        });

    return "";
}

QString streamlink(const CommandContext &ctx)
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
                "/streamlink [channel]. Open specified Twitch channel in "
                "streamlink. If no channel argument is specified, open the "
                "current Twitch channel instead."));
            return "";
        }
    }

    stripChannelName(target);
    openStreamlinkForChannel(target);

    return "";
}

QString popout(const CommandContext &ctx)
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
                "Usage: /popout <channel>. You can also use the command "
                "without arguments in any Twitch channel to open its "
                "popout chat."));
            return "";
        }
    }

    stripChannelName(target);
    QDesktopServices::openUrl(QUrl(
        QString("https://www.twitch.tv/popout/%1/chat?popout=").arg(target)));

    return "";
}

QString popup(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    static const auto *usageMessage =
        "Usage: /popup [channel]. Open specified Twitch channel in "
        "a new window. If no channel argument is specified, open "
        "the currently selected split instead.";

    QString target(ctx.words.value(1));
    stripChannelName(target);

    // Popup the current split
    if (target.isEmpty())
    {
        auto *currentPage = dynamic_cast<SplitContainer *>(
            getApp()->windows->getMainWindow().getNotebook().getSelectedPage());
        if (currentPage != nullptr)
        {
            auto *currentSplit = currentPage->getSelectedSplit();
            if (currentSplit != nullptr)
            {
                currentSplit->popup();

                return "";
            }
        }

        ctx.channel->addMessage(makeSystemMessage(usageMessage));
        return "";
    }

    // Open channel passed as argument in a popup
    auto *app = getApp();
    auto targetChannel = app->twitch->getOrAddChannel(target);
    app->windows->openInPopup(targetChannel);

    return "";
}

QString clearmessages(const CommandContext &ctx)
{
    (void)ctx;

    auto *currentPage = dynamic_cast<SplitContainer *>(
        getApp()->windows->getMainWindow().getNotebook().getSelectedPage());

    if (auto *split = currentPage->getSelectedSplit())
    {
        split->getChannelView().clearMessages();
    }

    return "";
}

QString openURL(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage("Usage: /openurl <URL>"));
        return "";
    }

    QUrl url = QUrl::fromUserInput(ctx.words.mid(1).join(" "));
    if (!url.isValid())
    {
        ctx.channel->addMessage(makeSystemMessage("Invalid URL specified."));
        return "";
    }

    bool res = false;
    if (supportsIncognitoLinks() && getSettings()->openLinksIncognito)
    {
        res = openLinkIncognito(url.toString(QUrl::FullyEncoded));
    }
    else
    {
        res = QDesktopServices::openUrl(url);
    }

    if (!res)
    {
        ctx.channel->addMessage(makeSystemMessage("Could not open URL."));
    }

    return "";
}

QString sendRawMessage(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.channel->isTwitchChannel())
    {
        getApp()->twitch->sendRawMessage(ctx.words.mid(1).join(" "));
    }
    else
    {
        // other code down the road handles this for IRC
        return ctx.words.join(" ");
    }
    return "";
}

QString injectFakeMessage(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (!ctx.channel->isTwitchChannel())
    {
        ctx.channel->addMessage(makeSystemMessage(
            "The /fakemsg command only works in Twitch channels."));
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "Usage: /fakemsg (raw irc text) - injects raw irc text as "
            "if it was a message received from TMI"));
        return "";
    }

    auto ircText = ctx.words.mid(1).join(" ");
    getApp()->twitch->addFakeMessage(ircText);

    return "";
}

QString copyToClipboard(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(
            makeSystemMessage("Usage: /copy <text> - copies provided "
                              "text to clipboard."));
        return "";
    }

    crossPlatformCopy(ctx.words.mid(1).join(" "));
    return "";
}

QString unstableSetUserClientSideColor(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(
            makeSystemMessage("The /unstable-set-user-color command only "
                              "works in Twitch channels."));
        return "";
    }
    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage(
            QString("Usage: %1 <TwitchUserID> [color]").arg(ctx.words.at(0))));
        return "";
    }

    auto userID = ctx.words.at(1);

    auto color = ctx.words.value(2);

    getIApp()->getUserData()->setUserColor(userID, color);

    return "";
}

QString openUsercard(const CommandContext &ctx)
{
    auto channel = ctx.channel;

    if (channel == nullptr)
    {
        return "";
    }

    if (ctx.words.size() < 2)
    {
        channel->addMessage(
            makeSystemMessage("Usage: /usercard <username> [channel] or "
                              "/usercard id:<id> [channel]"));
        return "";
    }

    QString userName = ctx.words[1];
    stripUserName(userName);

    if (ctx.words.size() > 2)
    {
        QString channelName = ctx.words[2];
        stripChannelName(channelName);

        ChannelPtr channelTemp =
            getApp()->twitch->getChannelOrEmpty(channelName);

        if (channelTemp->isEmpty())
        {
            channel->addMessage(makeSystemMessage(
                "A usercard can only be displayed for a channel that is "
                "currently opened in Chatterino."));
            return "";
        }

        channel = channelTemp;
    }

    // try to link to current split if possible
    Split *currentSplit = nullptr;
    auto *currentPage = dynamic_cast<SplitContainer *>(
        getApp()->windows->getMainWindow().getNotebook().getSelectedPage());
    if (currentPage != nullptr)
    {
        currentSplit = currentPage->getSelectedSplit();
    }

    auto differentChannel =
        currentSplit != nullptr && currentSplit->getChannel() != channel;
    if (differentChannel || currentSplit == nullptr)
    {
        // not possible to use current split, try searching for one
        const auto &notebook = getApp()->windows->getMainWindow().getNotebook();
        auto count = notebook.getPageCount();
        for (int i = 0; i < count; i++)
        {
            auto *page = notebook.getPageAt(i);
            auto *container = dynamic_cast<SplitContainer *>(page);
            assert(container != nullptr);
            for (auto *split : container->getSplits())
            {
                if (split->getChannel() == channel)
                {
                    currentSplit = split;
                    break;
                }
            }
        }

        // This would have crashed either way.
        assert(currentSplit != nullptr &&
               "something went HORRIBLY wrong with the /usercard "
               "command. It couldn't find a split for a channel which "
               "should be open.");
    }

    auto *userPopup =
        new UserInfoPopup(getSettings()->autoCloseUserPopup, currentSplit);
    userPopup->setData(userName, channel);
    userPopup->moveTo(QCursor::pos(), widgets::BoundsChecking::CursorPosition);
    userPopup->show();
    return "";
}

}  // namespace chatterino::commands
