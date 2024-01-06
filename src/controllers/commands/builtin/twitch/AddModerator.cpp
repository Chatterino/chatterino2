#include "controllers/commands/builtin/twitch/AddModerator.hpp"

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

QString addModerator(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "The /mod command only works in Twitch channels."));
        return "";
    }
    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "Usage: \"/mod <username>\" - Grant moderator status to a "
            "user. Use \"/mods\" to list the moderators of this channel."));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to mod someone!"));
        return "";
    }

    auto target = ctx.words.at(1);
    stripChannelName(target);

    getHelix()->getUserByName(
        target,
        [twitchChannel{ctx.twitchChannel},
         channel{ctx.channel}](const HelixUser &targetUser) {
            getHelix()->addChannelModerator(
                twitchChannel->roomId(), targetUser.id,
                [channel, targetUser] {
                    channel->addMessage(makeSystemMessage(
                        QString("You have added %1 as a moderator of this "
                                "channel.")
                            .arg(targetUser.displayName)));
                },
                [channel, targetUser](auto error, auto message) {
                    QString errorMessage =
                        QString("Failed to add channel moderator - ");

                    using Error = HelixAddChannelModeratorError;

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

                        case Error::TargetIsVIP: {
                            errorMessage +=
                                QString("%1 is currently a VIP, \"/unvip\" "
                                        "them and "
                                        "retry this command.")
                                    .arg(targetUser.displayName);
                        }
                        break;

                        case Error::TargetAlreadyModded: {
                            // Equivalent irc error
                            errorMessage =
                                QString("%1 is already a moderator of this "
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
