#include "controllers/commands/builtin/twitch/ChatSettings.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/api/Helix.hpp"
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
        channel->addMessage(makeSystemMessage(
            formatError(error, message, durationUnitMultiplier)));
    };
};

const auto P_NOT_LOGGED_IN =
    QStringLiteral("You must be logged in to update chat settings!");

}  // namespace

namespace chatterino::commands {

QString emoteOnly(const QStringList &words, ChannelPtr channel)
{
    (void)words;  // unused

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /emoteonly command only works in Twitch channels"));
        return "";
    }

    if (twitchChannel->accessRoomModes()->emoteOnly)
    {
        channel->addMessage(
            makeSystemMessage("This room is already in emote-only mode."));
        return "";
    }

    getHelix()->updateEmoteMode(twitchChannel->roomId(),
                                currentUser->getUserId(), true, successCallback,
                                failureCallback(channel));
    return "";
}

QString emoteOnlyOff(const QStringList &words, ChannelPtr channel)
{
    (void)words;  // unused

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }
    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /emoteonlyoff command only works in Twitch channels"));
        return "";
    }

    if (!twitchChannel->accessRoomModes()->emoteOnly)
    {
        channel->addMessage(
            makeSystemMessage("This room is not in emote-only mode."));
        return "";
    }

    getHelix()->updateEmoteMode(
        twitchChannel->roomId(), currentUser->getUserId(), false,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(makeSystemMessage(formatError(error, message)));
        });
    return "";
}

QString subscribers(const QStringList &words, ChannelPtr channel)
{
    (void)words;  // unused

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /subscribers command only works in Twitch channels"));
        return "";
    }

    if (twitchChannel->accessRoomModes()->submode)
    {
        channel->addMessage(makeSystemMessage(
            "This room is already in subscribers-only mode."));
        return "";
    }

    getHelix()->updateSubscriberMode(
        twitchChannel->roomId(), currentUser->getUserId(), true,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(makeSystemMessage(formatError(error, message)));
        });
    return "";
}

QString subscribersOff(const QStringList &words, ChannelPtr channel)
{
    (void)words;  // unused

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }
    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /subscribersoff command only works in Twitch channels"));
        return "";
    }

    if (!twitchChannel->accessRoomModes()->submode)
    {
        channel->addMessage(
            makeSystemMessage("This room is not in subscribers-only mode."));
        return "";
    }

    getHelix()->updateSubscriberMode(
        twitchChannel->roomId(), currentUser->getUserId(), false,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(makeSystemMessage(formatError(error, message)));
        });
    return "";
}

QString slow(const QStringList &words, ChannelPtr channel)
{
    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /slow command only works in Twitch channels"));
        return "";
    }

    int duration = 30;
    if (words.length() >= 2)
    {
        bool ok = false;
        duration = words.at(1).toInt(&ok);
        if (!ok || duration <= 0)
        {
            channel->addMessage(makeSystemMessage(
                "Usage: \"/slow [duration]\" - Enables slow mode (limit how "
                "often users may send messages). Duration (optional, "
                "default=30) must be a positive number of seconds. Use "
                "\"slowoff\" to disable."));
            return "";
        }
    }

    if (twitchChannel->accessRoomModes()->slowMode == duration)
    {
        channel->addMessage(makeSystemMessage(
            QString("This room is already in %1-second slow mode.")
                .arg(duration)));
        return "";
    }

    getHelix()->updateSlowMode(
        twitchChannel->roomId(), currentUser->getUserId(), duration,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(makeSystemMessage(formatError(error, message)));
        });
    return "";
}

QString slowOff(const QStringList &words, ChannelPtr channel)
{
    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }
    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /slowoff command only works in Twitch channels"));
        return "";
    }

    if (twitchChannel->accessRoomModes()->slowMode <= 0)
    {
        channel->addMessage(
            makeSystemMessage("This room is not in slow mode."));
        return "";
    }

    getHelix()->updateSlowMode(
        twitchChannel->roomId(), currentUser->getUserId(), boost::none,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(makeSystemMessage(formatError(error, message)));
        });
    return "";
}

QString followers(const QStringList &words, ChannelPtr channel)
{
    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /followers command only works in Twitch channels"));
        return "";
    }

    int duration = 0;
    if (words.length() >= 2)
    {
        auto parsed = parseDurationToSeconds(words.mid(1).join(' '), 60);
        duration = (int)(parsed / 60);
        // -1 / 60 == 0 => use parsed
        if (parsed < 0)
        {
            channel->addMessage(makeSystemMessage(
                "Usage: \"/followers [duration]\" - Enables followers-only "
                "mode (only users who have followed for 'duration' may chat). "
                "Examples: \"30m\", \"1 week\", \"5 days 12 hours\". Must be "
                "less than 3 months."));
            return "";
        }
    }

    if (twitchChannel->accessRoomModes()->followerOnly == duration)
    {
        channel->addMessage(makeSystemMessage(
            QString("This room is already in %1 followers-only mode.")
                .arg(formatTime(duration * 60))));
        return "";
    }

    getHelix()->updateFollowerMode(
        twitchChannel->roomId(), currentUser->getUserId(), duration,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(
                makeSystemMessage(formatError(error, message, 60)));
        });
    return "";
}

QString followersOff(const QStringList &words, ChannelPtr channel)
{
    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }
    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /followersoff command only works in Twitch channels"));
        return "";
    }

    if (twitchChannel->accessRoomModes()->followerOnly < 0)
    {
        channel->addMessage(
            makeSystemMessage("This room is not in followers-only mode. "));
        return "";
    }

    getHelix()->updateFollowerMode(
        twitchChannel->roomId(), currentUser->getUserId(), boost::none,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(makeSystemMessage(formatError(error, message)));
        });
    return "";
}

QString uniqueChat(const QStringList &words, ChannelPtr channel)
{
    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }
    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /uniquechat command only works in Twitch channels"));
        return "";
    }

    if (twitchChannel->accessRoomModes()->r9k)
    {
        channel->addMessage(
            makeSystemMessage("This room is already in unique-chat mode."));
        return "";
    }

    getHelix()->updateUniqueChatMode(
        twitchChannel->roomId(), currentUser->getUserId(), true,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(makeSystemMessage(formatError(error, message)));
        });

    return "";
}

QString uniqueChatOff(const QStringList &words, ChannelPtr channel)
{
    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(makeSystemMessage(P_NOT_LOGGED_IN));
        return "";
    }
    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /uniquechatoff command only works in Twitch channels"));
        return "";
    }

    if (!twitchChannel->accessRoomModes()->r9k)
    {
        channel->addMessage(
            makeSystemMessage("This room is not in unique-chat mode."));
        return "";
    }

    getHelix()->updateUniqueChatMode(
        twitchChannel->roomId(), currentUser->getUserId(), false,
        successCallback, [channel](auto error, auto message) {
            channel->addMessage(makeSystemMessage(formatError(error, message)));
        });
    return "";
}

}  // namespace chatterino::commands
