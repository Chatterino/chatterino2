#include "controllers/commands/builtin/twitch/Ban.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

#include <QCommandLineParser>

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

void banUserByID(const ChannelPtr &channel, const QString &channelID,
                 const QString &sourceUserID, const QString &targetUserID,
                 const QString &reason, const QString &displayName)
{
    getHelix()->banUser(
        channelID, sourceUserID, targetUserID, std::nullopt, reason,
        [] {
            // No response for bans, they're emitted over pubsub/IRC instead
        },
        [channel, displayName](auto error, auto message) {
            auto errorMessage =
                formatBanTimeoutError("ban", error, message, displayName);
            channel->addMessage(makeSystemMessage(errorMessage));
        });
}

void timeoutUserByID(const ChannelPtr &channel, const QString &channelID,
                     const QString &sourceUserID, const QString &targetUserID,
                     int duration, const QString &reason,
                     const QString &displayName)
{
    getHelix()->banUser(
        channelID, sourceUserID, targetUserID, duration, reason,
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

std::vector<PerformChannelAction> parseBanCommand(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        // A ban action must be performed with a channel as a context
        return {};
    }

    QCommandLineParser parser;
    parser.setOptionsAfterPositionalArgumentsMode(
        QCommandLineParser::ParseAsPositionalArguments);
    parser.addPositionalArgument("username", "The name of the user to ban");
    parser.addPositionalArgument("reason", "The optional ban reason");
    QCommandLineOption channelOption(
        "channel", "Override which channel(s) to perform the action in",
        "channel id");
    parser.addOptions({
        channelOption,
    });
    parser.parse(ctx.words);

    const auto &positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty())
    {
        return {};
    }

    const auto &rawTarget = positionalArguments.first();
    const auto reason = positionalArguments.sliced(1).join(' ');

    auto overrideChannels = parser.values(channelOption);
    if (overrideChannels.isEmpty())
    {
        if (ctx.twitchChannel == nullptr)
        {
            // No override channel specified, but the command was not run in a Twitch channel
            return {};
        }

        return {
            PerformChannelAction{
                .channelID = ctx.twitchChannel->roomId(),
                .rawTarget = rawTarget,
                .reason = reason,
            },
        };
    }

    std::vector<PerformChannelAction> actions;

    for (const auto &overrideChannel : overrideChannels)
    {
        actions.push_back(PerformChannelAction{
            .channelID = overrideChannel,
            .rawTarget = rawTarget,
            .reason = reason,
        });
    }

    return actions;
}

QString sendBan(const CommandContext &ctx)
{
    const auto actions = parseBanCommand(ctx);

    if (actions.empty())
    {
        // TODO: better error message
        return "";
    }

    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to ban someone!"));
        return "";
    }

    for (const auto &action : actions)
    {
        const auto &reason = action.reason;
        qInfo() << "banning" << action.rawTarget << "in" << action.channelID
                << "with reason" << action.reason;
        auto [targetUserName, targetUserID] =
            parseUserNameOrID(action.rawTarget);

        if (!targetUserID.isEmpty())
        {
            banUserByID(ctx.channel, action.channelID, currentUser->getUserId(),
                        targetUserID, reason, targetUserID);
        }
        else
        {
            getHelix()->getUserByName(
                targetUserName,
                [channel{ctx.channel}, channelID{action.channelID}, currentUser,
                 twitchChannel{ctx.twitchChannel},
                 reason](const auto &targetUser) {
                    banUserByID(channel, channelID, currentUser->getUserId(),
                                targetUser.id, reason, targetUser.displayName);
                },
                [channel{ctx.channel}, targetUserName{targetUserName}] {
                    // Equivalent error from IRC
                    channel->addMessage(makeSystemMessage(
                        QString("Invalid username: %1").arg(targetUserName)));
                });
        }
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

    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(
            makeSystemMessage("You must be logged in to ban someone!"));
        return "";
    }

    auto target = words.at(1);
    auto reason = words.mid(2).join(' ');

    banUserByID(channel, twitchChannel->roomId(), currentUser->getUserId(),
                target, reason, target);

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

    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
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
        timeoutUserByID(channel, twitchChannel->roomId(),
                        currentUser->getUserId(), targetUserID, duration,
                        reason, targetUserID);
    }
    else
    {
        getHelix()->getUserByName(
            targetUserName,
            [channel, currentUser, channelID{twitchChannel->roomId()},
             targetUserName{targetUserName}, duration,
             reason](const auto &targetUser) {
                timeoutUserByID(channel, channelID, currentUser->getUserId(),
                                targetUser.id, duration, reason,
                                targetUser.displayName);
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
