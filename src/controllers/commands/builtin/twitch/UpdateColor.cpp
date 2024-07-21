#include "controllers/commands/builtin/twitch/UpdateColor.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Twitch.hpp"

namespace chatterino::commands {

QString updateUserColor(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (!ctx.channel->isTwitchChannel())
    {
        ctx.channel->addSystemMessage(
            "The /color command only works in Twitch channels.");
        return "";
    }
    auto user = getApp()->getAccounts()->twitch.getCurrent();

    // Avoid Helix calls without Client ID and/or OAuth Token
    if (user->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to use the /color command.");
        return "";
    }

    auto colorString = ctx.words.value(1);

    if (colorString.isEmpty())
    {
        ctx.channel->addSystemMessage(
            QString("Usage: /color <color> - Color must be one of Twitch's "
                    "supported colors (%1) or a hex code (#000000) if you "
                    "have Turbo or Prime.")
                .arg(VALID_HELIX_COLORS.join(", ")));
        return "";
    }

    cleanHelixColorName(colorString);

    getHelix()->updateUserChatColor(
        user->getUserId(), colorString,
        [colorString, channel{ctx.channel}] {
            QString successMessage =
                QString("Your color has been changed to %1.").arg(colorString);
            channel->addSystemMessage(successMessage);
        },
        [colorString, channel{ctx.channel}](auto error, auto message) {
            QString errorMessage =
                QString("Failed to change color to %1 - ").arg(colorString);

            switch (error)
            {
                case HelixUpdateUserChatColorError::UserMissingScope: {
                    errorMessage +=
                        "Missing required scope. Re-login with your "
                        "account and try again.";
                }
                break;

                case HelixUpdateUserChatColorError::InvalidColor: {
                    errorMessage += QString("Color must be one of Twitch's "
                                            "supported colors (%1) or a "
                                            "hex code (#000000) if you "
                                            "have Turbo or Prime.")
                                        .arg(VALID_HELIX_COLORS.join(", "));
                }
                break;

                case HelixUpdateUserChatColorError::Forwarded: {
                    errorMessage += message + ".";
                }
                break;

                case HelixUpdateUserChatColorError::Unknown:
                default: {
                    errorMessage += "An unknown error has occurred.";
                }
                break;
            }

            channel->addSystemMessage(errorMessage);
        });

    return "";
}

}  // namespace chatterino::commands
