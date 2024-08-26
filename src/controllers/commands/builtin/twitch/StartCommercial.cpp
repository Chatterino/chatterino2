#include "controllers/commands/builtin/twitch/StartCommercial.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace {

using namespace chatterino;

QString formatStartCommercialError(HelixStartCommercialError error,
                                   const QString &message)
{
    using Error = HelixStartCommercialError;

    QString errorMessage = "Failed to start commercial - ";

    switch (error)
    {
        case Error::UserMissingScope: {
            errorMessage += "Missing required scope. Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::TokenMustMatchBroadcaster: {
            errorMessage += "Only the broadcaster of the channel can run "
                            "commercials.";
        }
        break;

        case Error::BroadcasterNotStreaming: {
            errorMessage += "You must be streaming live to run "
                            "commercials.";
        }
        break;

        case Error::MissingLengthParameter: {
            errorMessage += "Command must include a desired commercial break "
                            "length that is greater than zero.";
        }
        break;

        case Error::Ratelimited: {
            errorMessage += "You must wait until your cooldown period "
                            "expires before you can run another "
                            "commercial.";
        }
        break;

        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::Unknown:
        default: {
            errorMessage +=
                QString("An unknown error has occurred (%1).").arg(message);
        }
        break;
    }

    return errorMessage;
}

}  // namespace

namespace chatterino::commands {

QString startCommercial(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /commercial command only works in Twitch channels.");
        return "";
    }

    const auto *usageStr = "Usage: \"/commercial <length>\" - Starts a "
                           "commercial with the "
                           "specified duration for the current "
                           "channel. Valid length options "
                           "are 30, 60, 90, 120, 150, and 180 seconds.";

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage(usageStr);
        return "";
    }

    auto user = getApp()->getAccounts()->twitch.getCurrent();

    // Avoid Helix calls without Client ID and/or OAuth Token
    if (user->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to use the /commercial command.");
        return "";
    }

    auto broadcasterID = ctx.twitchChannel->roomId();
    auto length = ctx.words.at(1).toInt();

    getHelix()->startCommercial(
        broadcasterID, length,
        [channel{ctx.channel}](auto response) {
            channel->addSystemMessage(
                QString("Starting %1 second long commercial break. "
                        "Keep in mind you are still "
                        "live and not all viewers will receive a "
                        "commercial. "
                        "You may run another commercial in %2 seconds.")
                    .arg(response.length)
                    .arg(response.retryAfter));
        },
        [channel{ctx.channel}](auto error, auto message) {
            auto errorMessage = formatStartCommercialError(error, message);
            channel->addSystemMessage(errorMessage);
        });

    return "";
}

}  // namespace chatterino::commands
