#include "controllers/commands/builtin/twitch/Raid.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

namespace {

using namespace chatterino;

QString formatStartRaidError(HelixStartRaidError error, const QString &message)
{
    QString errorMessage = QString("Failed to start a raid - ");

    using Error = HelixStartRaidError;

    switch (error)
    {
        case Error::UserMissingScope: {
            // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            errorMessage += "You must be the broadcaster "
                            "to start a raid.";
        }
        break;

        case Error::CantRaidYourself: {
            errorMessage += "A channel cannot raid itself.";
        }
        break;

        case Error::Ratelimited: {
            errorMessage += "You are being ratelimited "
                            "by Twitch. Try "
                            "again in a few seconds.";
        }
        break;

        case Error::Forwarded: {
            errorMessage += message;
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

QString formatCancelRaidError(HelixCancelRaidError error,
                              const QString &message)
{
    QString errorMessage = QString("Failed to cancel the raid - ");

    using Error = HelixCancelRaidError;

    switch (error)
    {
        case Error::UserMissingScope: {
            // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            errorMessage += "You must be the broadcaster "
                            "to cancel the raid.";
        }
        break;

        case Error::NoRaidPending: {
            errorMessage += "You don't have an active raid.";
        }
        break;

        case Error::Ratelimited: {
            errorMessage += "You are being ratelimited by Twitch. Try "
                            "again in a few seconds.";
        }
        break;

        case Error::Forwarded: {
            errorMessage += message;
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

}  // namespace

namespace chatterino::commands {

QString startRaid(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /raid command only works in Twitch channels.");
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage(
            "Usage: \"/raid <username>\" - Raid a user. "
            "Only the broadcaster can start a raid.");
        return "";
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage("You must be logged in to start a raid!");
        return "";
    }

    auto target = ctx.words.at(1);
    stripChannelName(target);

    getHelix()->getUserByName(
        target,
        [twitchChannel{ctx.twitchChannel},
         channel{ctx.channel}](const HelixUser &targetUser) {
            getHelix()->startRaid(
                twitchChannel->roomId(), targetUser.id,
                [] {
                    // do nothing
                },
                [channel, targetUser](auto error, auto message) {
                    auto errorMessage = formatStartRaidError(error, message);
                    channel->addSystemMessage(errorMessage);
                });
        },
        [channel{ctx.channel}, target] {
            // Equivalent error from IRC
            channel->addSystemMessage(
                QString("Invalid username: %1").arg(target));
        });

    return "";
}

QString cancelRaid(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /unraid command only works in Twitch channels.");
        return "";
    }

    if (ctx.words.size() != 1)
    {
        ctx.channel->addSystemMessage(
            "Usage: \"/unraid\" - Cancel the current raid. "
            "Only the broadcaster can cancel the raid.");
        return "";
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to cancel the raid!");
        return "";
    }

    getHelix()->cancelRaid(
        ctx.twitchChannel->roomId(),
        [] {
            // do nothing
        },
        [channel{ctx.channel}](auto error, auto message) {
            auto errorMessage = formatCancelRaidError(error, message);
            channel->addSystemMessage(errorMessage);
        });

    return "";
}

}  // namespace chatterino::commands
