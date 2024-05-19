#include "controllers/commands/builtin/twitch/Ban.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

#include <QCommandLineParser>
#include <QStringBuilder>

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

nonstd::expected<std::vector<PerformChannelAction>, QString> parseChannelAction(
    const CommandContext &ctx, const QString &command, const QString &usage,
    bool withDuration)
{
    if (ctx.channel == nullptr)
    {
        // A ban action must be performed with a channel as a context
        return nonstd::make_unexpected(
            "A " % command %
            " action must be performed with a channel as a context");
    }

    QCommandLineParser parser;
    parser.setOptionsAfterPositionalArgumentsMode(
        QCommandLineParser::ParseAsPositionalArguments);
    parser.addPositionalArgument("username", "The name of the user to ban");
    if (withDuration)
    {
        parser.addPositionalArgument("duration", "Duration of the action");
    }
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
        return nonstd::make_unexpected("Missing target - " % usage);
    }

    PerformChannelAction base{
        .rawTarget = positionalArguments.first(),
        .duration = 0,
    };

    if (withDuration)
    {
        auto durationStr = positionalArguments.value(1);
        if (durationStr.isEmpty())
        {
            base.duration = 10 * 60;  // 10 min
        }
        else
        {
            base.duration = (int)parseDurationToSeconds(durationStr);
            if (base.duration <= 0)
            {
                return nonstd::make_unexpected("Invalid duration - " % usage);
            }
        }
        base.reason = positionalArguments.sliced(2).join(' ');
    }
    else
    {
        base.reason = positionalArguments.sliced(1).join(' ');
    }

    std::vector<PerformChannelAction> actions;

    auto overrideChannels = parser.values(channelOption);
    if (overrideChannels.isEmpty())
    {
        if (ctx.twitchChannel == nullptr)
        {
            return nonstd::make_unexpected(
                "The " % command % " command only works in Twitch channels");
        }

        actions.push_back(PerformChannelAction{
            .channelID = ctx.twitchChannel->roomId(),
            .rawTarget = base.rawTarget,
            .reason = base.reason,
            .duration = base.duration,
        });
    }
    else
    {
        for (const auto &overrideChannel : overrideChannels)
        {
            actions.push_back(PerformChannelAction{
                .channelID = overrideChannel,
                .rawTarget = base.rawTarget,
                .reason = base.reason,
                .duration = base.duration,
            });
        }
    }

    return actions;
}

QString sendBan(const CommandContext &ctx)
{
    const auto command = QStringLiteral("/ban");
    const auto usage = QStringLiteral(
        R"(Usage: "/ban [options...] <username> [reason]" - Permanently prevent a user from chatting via their username. Reason is optional and will be shown to the target user and other moderators. Options: --channel <channelID> to override which channel the ban takes place in (can be specified multiple times).)");
    const auto actions = parseChannelAction(ctx, command, usage, false);

    if (!actions.has_value())
    {
        if (ctx.channel != nullptr)
        {
            ctx.channel->addMessage(makeSystemMessage(actions.error()));
        }
        else
        {
            qCWarning(chatterinoCommands)
                << "Error parsing command:" << actions.error();
        }

        return "";
    }

    assert(!actions.value().empty());

    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to ban someone!"));
        return "";
    }

    for (const auto &action : actions.value())
    {
        const auto &reason = action.reason;
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
    const auto command = QStringLiteral("/banid");
    const auto usage = QStringLiteral(
        R"(Usage: "/banid [options...] <userID> [reason]" - Permanently prevent a user from chatting via their user ID. Reason is optional and will be shown to the target user and other moderators. Options: --channel <channelID> to override which channel the ban takes place in (can be specified multiple times).)");
    const auto actions = parseChannelAction(ctx, command, usage, false);

    if (!actions.has_value())
    {
        if (ctx.channel != nullptr)
        {
            ctx.channel->addMessage(makeSystemMessage(actions.error()));
        }
        else
        {
            qCWarning(chatterinoCommands)
                << "Error parsing command:" << actions.error();
        }

        return "";
    }

    assert(!actions.value().empty());

    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to ban someone!"));
        return "";
    }

    for (const auto &action : actions.value())
    {
        const auto &reason = action.reason;
        const auto &targetUserID = action.rawTarget;
        banUserByID(ctx.channel, action.channelID, currentUser->getUserId(),
                    targetUserID, reason, targetUserID);
    }

    return "";
}

QString sendTimeout(const CommandContext &ctx)
{
    const auto command = QStringLiteral("/timeout");
    const auto usage = QStringLiteral(
        R"(Usage: "/timeout [options...] <username> [duration][time unit] [reason]" - Temporarily prevent a user from chatting. Duration (optional, default=10 minutes) must be a positive integer; time unit (optional, default=s) must be one of s, m, h, d, w; maximum duration is 2 weeks. Combinations like 1d2h are also allowed. Reason is optional and will be shown to the target user and other moderators. Use \"/untimeout\" to remove a timeout. Options: --channel <channelID> to override which channel the ban takes place in (can be specified multiple times).)");
    const auto actions = parseChannelAction(ctx, command, usage, true);

    if (!actions.has_value())
    {
        if (ctx.channel != nullptr)
        {
            ctx.channel->addMessage(makeSystemMessage(actions.error()));
        }
        else
        {
            qCWarning(chatterinoCommands)
                << "Error parsing command:" << actions.error();
        }

        return "";
    }

    assert(!actions.value().empty());

    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to ban someone!"));
        return "";
    }

    for (const auto &action : actions.value())
    {
        const auto &reason = action.reason;
        const auto duration = action.duration;
        auto [targetUserName, targetUserID] =
            parseUserNameOrID(action.rawTarget);

        if (!targetUserID.isEmpty())
        {
            timeoutUserByID(ctx.channel, action.channelID,
                            currentUser->getUserId(), targetUserID, duration,
                            reason, targetUserID);
        }
        else
        {
            getHelix()->getUserByName(
                targetUserName,
                [channel{ctx.channel}, channelID{action.channelID}, currentUser,
                 duration, reason](const auto &targetUser) {
                    timeoutUserByID(channel, channelID,
                                    currentUser->getUserId(), targetUser.id,
                                    duration, reason, targetUser.displayName);
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

}  // namespace chatterino::commands
