#include "controllers/commands/builtin/twitch/ShieldMode.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino::commands {

QString toggleShieldMode(const CommandContext &ctx, bool isActivating)
{
    const QString command =
        isActivating ? QStringLiteral("/shield") : QStringLiteral("/shieldoff");

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(makeSystemMessage(
            QStringLiteral("The %1 command only works in Twitch channels.")
                .arg(command)));
        return {};
    }

    auto user = getApp()->accounts->twitch.getCurrent();

    // Avoid Helix calls without Client ID and/or OAuth Token
    if (user->isAnon())
    {
        ctx.channel->addMessage(makeSystemMessage(
            QStringLiteral("You must be logged in to use the %1 command.")
                .arg(command)));
        return {};
    }

    getHelix()->updateShieldMode(
        ctx.twitchChannel->roomId(), user->getUserId(), isActivating,
        [channel = ctx.channel](const auto &res) {
            if (!res.isActive)
            {
                channel->addMessage(
                    makeSystemMessage("Shield mode was deactivated."));
                return;
            }

            channel->addMessage(
                makeSystemMessage("Shield mode was activated."));
        },
        [channel = ctx.channel](const auto error, const auto &message) {
            using Error = HelixUpdateShieldModeError;
            QString errorMessage = "Failed to update shield mode - ";

            switch (error)
            {
                case Error::UserMissingScope: {
                    errorMessage +=
                        "Missing required scope. Re-login with your "
                        "account and try again.";
                }
                break;

                case Error::MissingPermission: {
                    errorMessage += "You must be a moderator of the channel.";
                }
                break;

                case Error::Forwarded: {
                    errorMessage += message;
                }
                break;

                case Error::Unknown:
                default: {
                    errorMessage +=
                        QString("An unknown error has occurred (%1).")
                            .arg(message);
                }
                break;
            }
            channel->addMessage(makeSystemMessage(errorMessage));
        });

    return {};
}

QString shieldModeOn(const CommandContext &ctx)
{
    return toggleShieldMode(ctx, true);
}

QString shieldModeOff(const CommandContext &ctx)
{
    return toggleShieldMode(ctx, false);
}

}  // namespace chatterino::commands
