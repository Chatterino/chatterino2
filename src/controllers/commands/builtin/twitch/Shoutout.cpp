#include "controllers/commands/builtin/twitch/Shoutout.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

namespace chatterino::commands {

QString sendShoutout(const CommandContext &ctx)
{
    auto *twitchChannel = ctx.twitchChannel;
    auto channel = ctx.channel;
    const auto *words = &ctx.words;

    if (twitchChannel == nullptr)
    {
        channel->addMessage(makeSystemMessage(
            "The /shoutout command only works in Twitch channels."));
        return "";
    }

    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(
            makeSystemMessage("You must be logged in to send shoutout."));
        return "";
    }

    if (words->size() < 2)
    {
        channel->addMessage(
            makeSystemMessage("Usage: \"/shoutout <username>\" - Sends a "
                              "shoutout to the specified twitch user"));
        return "";
    }

    auto target = words->at(1);
    stripChannelName(target);

    using Error = HelixSendShoutoutError;

    getHelix()->getUserByName(
        target,
        [twitchChannel, channel, currentUser](const auto targetUser) {
            getHelix()->sendShoutout(
                twitchChannel->roomId(), targetUser.id,
                currentUser->getUserId(),
                [channel, targetUser]() {
                    channel->addMessage(makeSystemMessage(
                        QString("Sent shoutout to %1").arg(targetUser.login)));
                },
                [channel](auto error, auto message) {
                    QString errorMessage = "Failed to send shoutout - ";

                    switch (error)
                    {
                        case Error::UserNotAuthorized: {
                            errorMessage += "You don't have permission to "
                                            "perform that action.";
                        }
                        break;

                        case Error::UserMissingScope: {
                            errorMessage += "Missing required scope. "
                                            "Re-login with your "
                                            "account and try again.";
                        }
                        break;

                        case Error::Ratelimited: {
                            errorMessage +=
                                "You are being ratelimited by Twitch. "
                                "Try again in a few seconds.";
                        }
                        break;

                        case Error::UserIsBroadcaster: {
                            errorMessage += "The broadcaster may not give "
                                            "themselves a Shoutout.";
                        }
                        break;

                        case Error::BroadcasterNotLive: {
                            errorMessage +=
                                "The broadcaster is not streaming live or "
                                "does not have one or more viewers.";
                        }
                        break;

                        case Error::Unknown: {
                            errorMessage += message;
                        }
                        break;
                    }

                    channel->addMessage(makeSystemMessage(errorMessage));
                });
        },
        [channel, target] {
            // Equivalent error from IRC
            channel->addMessage(
                makeSystemMessage(QString("Invalid username: %1").arg(target)));
        });

    return "";
}

}  // namespace chatterino::commands
