#include "controllers/commands/builtin/twitch/Ban.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

namespace {

using namespace chatterino;

QString formatBanTimeoutError(const char *operation, HelixBanUserError error,
                              const QString &message, const QString &userTarget)
{
    using Error = HelixBanUserError;

    QString errorMessage = QString("Failed to %1 user - ").arg(operation);

    switch (error)
    {
        case Error::ConflictingOperation: {
            errorMessage += "There was a conflicting ban operation on "
                            "this user. Please try again.";
        }
        break;

        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::Ratelimited: {
            errorMessage += "You are being ratelimited by Twitch. Try "
                            "again in a few seconds.";
        }
        break;

        case Error::TargetBanned: {
            // Equivalent IRC error
            errorMessage += QString("%1 is already banned in this channel.")
                                .arg(userTarget);
        }
        break;

        case Error::CannotBanUser: {
            // We can't provide the identical error as in IRC,
            // because we don't have enough information about the user.
            // The messages from IRC are formatted like this:
            // "You cannot {op} moderator {mod} unless you are the owner of this channel."
            // "You cannot {op} the broadcaster."
            errorMessage +=
                QString("You cannot %1 %2.").arg(operation, userTarget);
        }
        break;

        case Error::UserMissingScope: {
            // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            // TODO(pajlada): Phrase MISSING_PERMISSION
            errorMessage += "You don't have permission to "
                            "perform that action.";
        }
        break;

        case Error::Unknown: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

void banUserByID(const ChannelPtr &channel, const TwitchChannel *twitchChannel,
                 const QString &sourceUserID, const QString &targetUserID,
                 const QString &reason, const QString &displayName)
{
    getHelix()->banUser(
        twitchChannel->roomId(), sourceUserID, targetUserID, std::nullopt,
        reason,
        [] {
            // No response for bans, they're emitted over pubsub/IRC instead
        },
        [channel, displayName](auto error, auto message) {
            auto errorMessage =
                formatBanTimeoutError("ban", error, message, displayName);
            channel->addMessage(makeSystemMessage(errorMessage));
        });
}

void timeoutUserByID(const ChannelPtr &channel,
                     const TwitchChannel *twitchChannel,
                     const QString &sourceUserID, const QString &targetUserID,
                     int duration, const QString &reason,
                     const QString &displayName)
{
    getHelix()->banUser(
        twitchChannel->roomId(), sourceUserID, targetUserID, duration, reason,
        [] {
            // No response for timeouts, they're emitted over pubsub/IRC instead
        },
        [channel, displayName](auto error, auto message) {
            auto errorMessage =
                formatBanTimeoutError("timeout", error, message, displayName);
            channel->addMessage(makeSystemMessage(errorMessage));
        });
}

}  // namespace

namespace chatterino::commands {

QString sendBan(const CommandContext &ctx)
{
    const auto &words = ctx.words;
    const auto &channel = ctx.channel;
    const auto *twitchChannel = ctx.twitchChannel;

    if (channel == nullptr)
    {
        return "";
    }

    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            QString("The /ban command only works in Twitch channels.")));
        return "";
    }

    const auto *usageStr =
        "Usage: \"/ban <username> [reason]\" - Permanently prevent a user "
        "from chatting. Reason is optional and will be shown to the target "
        "user and other moderators. Use \"/unban\" to remove a ban.";
    if (words.size() < 2)
    {
        channel->addMessage(makeSystemMessage(usageStr));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(
            makeSystemMessage("You must be logged in to ban someone!"));
        return "";
    }

    const auto &rawTarget = words.at(1);
    auto [targetUserName, targetUserID] = parseUserNameOrID(rawTarget);
    auto reason = words.mid(2).join(' ');

    if (!targetUserID.isEmpty())
    {
        banUserByID(channel, twitchChannel, currentUser->getUserId(),
                    targetUserID, reason, targetUserID);
    }
    else
    {
        getHelix()->getUserByName(
            targetUserName,
            [channel, currentUser, twitchChannel,
             reason](const auto &targetUser) {
                banUserByID(channel, twitchChannel, currentUser->getUserId(),
                            targetUser.id, reason, targetUser.displayName);
            },
            [channel, targetUserName{targetUserName}] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(targetUserName)));
            });
    }

    return "";
}

QString sendBanById(const CommandContext &ctx)
{
    const auto &words = ctx.words;
    const auto &channel = ctx.channel;
    const auto *twitchChannel = ctx.twitchChannel;

    if (channel == nullptr)
    {
        return "";
    }
    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            QString("The /banid command only works in Twitch channels.")));
        return "";
    }

    const auto *usageStr =
        "Usage: \"/banid <userID> [reason]\" - Permanently prevent a user "
        "from chatting via their user ID. Reason is optional and will be "
        "shown to the target user and other moderators.";
    if (words.size() < 2)
    {
        channel->addMessage(makeSystemMessage(usageStr));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(
            makeSystemMessage("You must be logged in to ban someone!"));
        return "";
    }

    auto target = words.at(1);
    auto reason = words.mid(2).join(' ');

    banUserByID(channel, twitchChannel, currentUser->getUserId(), target,
                reason, target);

    return "";
}

QString sendTimeout(const CommandContext &ctx)
{
    const auto &words = ctx.words;
    const auto &channel = ctx.channel;
    const auto *twitchChannel = ctx.twitchChannel;

    if (channel == nullptr)
    {
        return "";
    }

    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            QString("The /timeout command only works in Twitch channels.")));
        return "";
    }
    const auto *usageStr =
        "Usage: \"/timeout <username> [duration][time unit] [reason]\" - "
        "Temporarily prevent a user from chatting. Duration (optional, "
        "default=10 minutes) must be a positive integer; time unit "
        "(optional, default=s) must be one of s, m, h, d, w; maximum "
        "duration is 2 weeks. Combinations like 1d2h are also allowed. "
        "Reason is optional and will be shown to the target user and other "
        "moderators. Use \"/untimeout\" to remove a timeout.";
    if (words.size() < 2)
    {
        channel->addMessage(makeSystemMessage(usageStr));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(
            makeSystemMessage("You must be logged in to timeout someone!"));
        return "";
    }

    const auto &rawTarget = words.at(1);
    auto [targetUserName, targetUserID] = parseUserNameOrID(rawTarget);

    int duration = 10 * 60;  // 10min
    if (words.size() >= 3)
    {
        duration = (int)parseDurationToSeconds(words.at(2));
        if (duration <= 0)
        {
            channel->addMessage(makeSystemMessage(usageStr));
            return "";
        }
    }
    auto reason = words.mid(3).join(' ');

    if (!targetUserID.isEmpty())
    {
        timeoutUserByID(channel, twitchChannel, currentUser->getUserId(),
                        targetUserID, duration, reason, targetUserID);
    }
    else
    {
        getHelix()->getUserByName(
            targetUserName,
            [channel, currentUser, twitchChannel,
             targetUserName{targetUserName}, duration,
             reason](const auto &targetUser) {
                timeoutUserByID(channel, twitchChannel,
                                currentUser->getUserId(), targetUser.id,
                                duration, reason, targetUser.displayName);
            },
            [channel, targetUserName{targetUserName}] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(targetUserName)));
            });
    }

    return "";
}

}  // namespace chatterino::commands
