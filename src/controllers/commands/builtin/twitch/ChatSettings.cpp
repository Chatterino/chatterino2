#include "controllers/commands/builtin/twitch/ChatSettings.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"

namespace {

using namespace chatterino;

QString formatError(const HelixUpdateChatSettingsError error,
                    const QString &message, int durationUnitMultiplier = 1)
{
    static const QRegularExpression invalidRange("(\\d+) through (\\d+)");

    QString errorMessage = QString("Failed to update - ");
    using Error = HelixUpdateChatSettingsError;
    switch (error)
    {
        case Error::UserMissingScope: {
            // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized:
        case Error::Forbidden: {
            // TODO(pajlada): Phrase MISSING_PERMISSION
            errorMessage += "You don't have permission to "
                            "perform that action.";
        }
        break;

        case Error::Ratelimited: {
            errorMessage += "You are being ratelimited by Twitch. Try "
                            "again in a few seconds.";
        }
        break;

        case Error::OutOfRange: {
            QRegularExpressionMatch matched = invalidRange.match(message);
            if (matched.hasMatch())
            {
                auto from = matched.captured(1).toInt();
                auto to = matched.captured(2).toInt();
                errorMessage +=
                    QString("The duration is out of the valid range: "
                            "%1 through %2.")
                        .arg(from == 0
                                 ? "0s"
                                 : formatTime(from * durationUnitMultiplier),
                             to == 0 ? "0s"
                                     : formatTime(to * durationUnitMultiplier));
            }
            else
            {
                errorMessage += message;
            }
        }
        break;

        case Error::Forwarded: {
            errorMessage = message;
        }
        break;

        case Error::Unknown:
        default: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

// Do nothing as we'll receive a message from IRC about updates
auto successCallback = [](auto result) {};

auto failureCallback = [](ChannelPtr channel, int durationUnitMultiplier = 1) {
    return [channel, durationUnitMultiplier](const auto &error,
                                             const QString &message) {
        channel->addSystemMessage(
            formatError(error, message, durationUnitMultiplier));
    };
};

const auto P_NOT_LOGGED_IN =
    QStringLiteral("You must be logged in to update chat settings!");

}  // namespace

namespace chatterino::commands {

QString emoteOnly(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /emoteonly command only works in Twitch channels.");
        return "";
    }

    if (ctx.twitchChannel->accessRoomModes()->emoteOnly)
    {
        ctx.channel->addSystemMessage(
            "This room is already in emote-only mode.");
        return "";
    }

    getHelix()->updateEmoteMode(ctx.twitchChannel->roomId(),
                                currentUser->getUserId(), true, successCallback,
                                failureCallback(ctx.channel));

    return "";
}

QString emoteOnlyOff(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }
    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /emoteonlyoff command only works in Twitch channels.");
        return "";
    }

    if (!ctx.twitchChannel->accessRoomModes()->emoteOnly)
    {
        ctx.channel->addSystemMessage("This room is not in emote-only mode.");
        return "";
    }

    getHelix()->updateEmoteMode(ctx.twitchChannel->roomId(),
                                currentUser->getUserId(), false,
                                successCallback, failureCallback(ctx.channel));

    return "";
}

QString subscribers(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /subscribers command only works in Twitch channels.");
        return "";
    }

    if (ctx.twitchChannel->accessRoomModes()->submode)
    {
        ctx.channel->addSystemMessage(
            "This room is already in subscribers-only mode.");
        return "";
    }

    getHelix()->updateSubscriberMode(
        ctx.twitchChannel->roomId(), currentUser->getUserId(), true,
        successCallback, failureCallback(ctx.channel));

    return "";
}

QString subscribersOff(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /subscribersoff command only works in Twitch channels.");
        return "";
    }

    if (!ctx.twitchChannel->accessRoomModes()->submode)
    {
        ctx.channel->addSystemMessage(
            "This room is not in subscribers-only mode.");
        return "";
    }

    getHelix()->updateSubscriberMode(
        ctx.twitchChannel->roomId(), currentUser->getUserId(), false,
        successCallback, failureCallback(ctx.channel));

    return "";
}

QString slow(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /slow command only works in Twitch channels.");
        return "";
    }

    int duration = 30;
    if (ctx.words.length() >= 2)
    {
        bool ok = false;
        duration = ctx.words.at(1).toInt(&ok);
        if (!ok || duration <= 0)
        {
            ctx.channel->addSystemMessage(
                "Usage: \"/slow [duration]\" - Enables slow mode (limit how "
                "often users may send messages). Duration (optional, "
                "default=30) must be a positive number of seconds. Use "
                "\"slowoff\" to disable.");
            return "";
        }
    }

    if (ctx.twitchChannel->accessRoomModes()->slowMode == duration)
    {
        ctx.channel->addSystemMessage(
            QString("This room is already in %1-second slow mode.")
                .arg(duration));
        return "";
    }

    getHelix()->updateSlowMode(ctx.twitchChannel->roomId(),
                               currentUser->getUserId(), duration,
                               successCallback, failureCallback(ctx.channel));

    return "";
}

QString slowOff(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /slowoff command only works in Twitch channels.");
        return "";
    }

    if (ctx.twitchChannel->accessRoomModes()->slowMode <= 0)
    {
        ctx.channel->addSystemMessage("This room is not in slow mode.");
        return "";
    }

    getHelix()->updateSlowMode(ctx.twitchChannel->roomId(),
                               currentUser->getUserId(), std::nullopt,
                               successCallback, failureCallback(ctx.channel));

    return "";
}

QString followers(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /followers command only works in Twitch channels.");
        return "";
    }

    int duration = 0;
    if (ctx.words.length() >= 2)
    {
        auto parsed = parseDurationToSeconds(ctx.words.mid(1).join(' '), 60);
        duration = (int)(parsed / 60);
        // -1 / 60 == 0 => use parsed
        if (parsed < 0)
        {
            ctx.channel->addSystemMessage(
                "Usage: \"/followers [duration]\" - Enables followers-only "
                "mode (only users who have followed for 'duration' may chat). "
                "Examples: \"30m\", \"1 week\", \"5 days 12 hours\". Must be "
                "less than 3 months.");
            return "";
        }
    }

    if (ctx.twitchChannel->accessRoomModes()->followerOnly == duration)
    {
        ctx.channel->addSystemMessage(
            QString("This room is already in %1 followers-only mode.")
                .arg(formatTime(duration * 60)));
        return "";
    }

    getHelix()->updateFollowerMode(
        ctx.twitchChannel->roomId(), currentUser->getUserId(), duration,
        successCallback, failureCallback(ctx.channel, 60));

    return "";
}

QString followersOff(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /followersoff command only works in Twitch channels.");
        return "";
    }

    if (ctx.twitchChannel->accessRoomModes()->followerOnly < 0)
    {
        ctx.channel->addSystemMessage(
            "This room is not in followers-only mode. ");
        return "";
    }

    getHelix()->updateFollowerMode(
        ctx.twitchChannel->roomId(), currentUser->getUserId(), std::nullopt,
        successCallback, failureCallback(ctx.channel));

    return "";
}

QString uniqueChat(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /uniquechat command only works in Twitch channels.");
        return "";
    }

    if (ctx.twitchChannel->accessRoomModes()->r9k)
    {
        ctx.channel->addSystemMessage(
            "This room is already in unique-chat mode.");
        return "";
    }

    getHelix()->updateUniqueChatMode(
        ctx.twitchChannel->roomId(), currentUser->getUserId(), true,
        successCallback, failureCallback(ctx.channel));

    return "";
}

QString uniqueChatOff(const CommandContext &ctx)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(P_NOT_LOGGED_IN);
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /uniquechatoff command only works in Twitch channels.");
        return "";
    }

    if (!ctx.twitchChannel->accessRoomModes()->r9k)
    {
        ctx.channel->addSystemMessage("This room is not in unique-chat mode.");
        return "";
    }

    getHelix()->updateUniqueChatMode(
        ctx.twitchChannel->roomId(), currentUser->getUserId(), false,
        successCallback, failureCallback(ctx.channel));

    return "";
}

}  // namespace chatterino::commands
