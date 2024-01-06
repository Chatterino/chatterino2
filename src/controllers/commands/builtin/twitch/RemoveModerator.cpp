#include "controllers/commands/builtin/twitch/RemoveModerator.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "util/Twitch.hpp"

namespace chatterino::commands {

QString removeModerator(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "The /unmod command only works in Twitch channels."));
        return "";
    }
    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "Usage: \"/unmod <username>\" - Revoke moderator status from a "
            "user. Use \"/mods\" to list the moderators of this channel."));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to unmod someone!"));
        return "";
    }

    auto target = ctx.words.at(1);
    stripChannelName(target);

    getHelix()->getUserByName(
        target,
        [twitchChannel{ctx.twitchChannel},
         channel{ctx.channel}](const HelixUser &targetUser) {
            getHelix()->removeChannelModerator(
                twitchChannel->roomId(), targetUser.id,
                [channel, targetUser] {
                    channel->addMessage(makeSystemMessage(
                        QString("You have removed %1 as a moderator of "
                                "this channel.")
                            .arg(targetUser.displayName)));
                },
                [channel, targetUser](auto error, auto message) {
                    QString errorMessage =
                        QString("Failed to remove channel moderator - ");

                    using Error = HelixRemoveChannelModeratorError;

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
                            // TODO(pajlada): Phrase MISSING_PERMISSION
                            errorMessage += "You don't have permission to "
                                            "perform that action.";
                        }
                        break;

                        case Error::Ratelimited: {
                            errorMessage +=
                                "You are being ratelimited by Twitch. Try "
                                "again in a few seconds.";
                        }
                        break;

                        case Error::TargetNotModded: {
                            // Equivalent irc error
                            errorMessage +=
                                QString("%1 is not a moderator of this "
                                        "channel.")
                                    .arg(targetUser.displayName);
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
                    channel->addMessage(makeSystemMessage(errorMessage));
                });
        },
        [channel{ctx.channel}, target] {
            // Equivalent error from IRC
            channel->addMessage(
                makeSystemMessage(QString("Invalid username: %1").arg(target)));
        });

    return "";
}

}  // namespace chatterino::commands
