#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/builtin/twitch/Ban.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

namespace chatterino::commands {

QString unbanUser(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    auto commandName = ctx.words.at(0).toLower();
    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(makeSystemMessage(
            QString("The %1 command only works in Twitch channels")
                .arg(commandName)));
        return "";
    }
    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage(
            QString("Usage: \"%1 <username>\" - Removes a ban on a user.")
                .arg(commandName)));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to unban someone!"));
        return "";
    }

    auto target = ctx.words.at(1);
    stripChannelName(target);

    getHelix()->getUserByName(
        target,
        [channel{ctx.channel}, currentUser, twitchChannel{ctx.twitchChannel},
         target](const auto &targetUser) {
            getHelix()->unbanUser(
                twitchChannel->roomId(), currentUser->getUserId(),
                targetUser.id,
                [] {
                    // No response for unbans, they're emitted over pubsub/IRC instead
                },
                [channel, target, targetUser](auto error, auto message) {
                    using Error = HelixUnbanUserError;

                    QString errorMessage = QString("Failed to unban user - ");

                    switch (error)
                    {
                        case Error::ConflictingOperation: {
                            errorMessage +=
                                "There was a conflicting ban operation on "
                                "this user. Please try again.";
                        }
                        break;

                        case Error::Forwarded: {
                            errorMessage += message;
                        }
                        break;

                        case Error::Ratelimited: {
                            errorMessage +=
                                "You are being ratelimited by Twitch. Try "
                                "again in a few seconds.";
                        }
                        break;

                        case Error::TargetNotBanned: {
                            // Equivalent IRC error
                            errorMessage =
                                QString("%1 is not banned from this channel.")
                                    .arg(targetUser.displayName);
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
