#include "controllers/commands/builtin/twitch/DeleteMessages.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/network/NetworkResult.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QUuid>

namespace {

using namespace chatterino;

QString deleteMessages(TwitchChannel *twitchChannel, const QString &messageID)
{
    const auto *commandName = messageID.isEmpty() ? "/clear" : "/delete";

    auto user = getApp()->getAccounts()->twitch.getCurrent();

    // Avoid Helix calls without Client ID and/or OAuth Token
    if (user->isAnon())
    {
        twitchChannel->addSystemMessage(
            QString("You must be logged in to use the %1 command.")
                .arg(commandName));
        return "";
    }

    getHelix()->deleteChatMessages(
        twitchChannel->roomId(), user->getUserId(), messageID,
        []() {
            // Success handling, we do nothing: IRC/pubsub-edge will dispatch the correct
            // events to update state for us.
        },
        [twitchChannel, messageID](auto error, auto message) {
            QString errorMessage = QString("Failed to delete chat messages - ");

            switch (error)
            {
                case HelixDeleteChatMessagesError::UserMissingScope: {
                    errorMessage +=
                        "Missing required scope. Re-login with your "
                        "account and try again.";
                }
                break;

                case HelixDeleteChatMessagesError::UserNotAuthorized: {
                    errorMessage +=
                        "you don't have permission to perform that action.";
                }
                break;

                case HelixDeleteChatMessagesError::MessageUnavailable: {
                    // Override default message prefix to match with IRC message format
                    errorMessage =
                        QString("The message %1 does not exist, was deleted, "
                                "or is too old to be deleted.")
                            .arg(messageID);
                }
                break;

                case HelixDeleteChatMessagesError::UserNotAuthenticated: {
                    errorMessage += "you need to re-authenticate.";
                }
                break;

                case HelixDeleteChatMessagesError::Forwarded: {
                    errorMessage += message;
                }
                break;

                case HelixDeleteChatMessagesError::Unknown:
                default: {
                    errorMessage += "An unknown error has occurred.";
                }
                break;
            }

            twitchChannel->addSystemMessage(errorMessage);
        });

    return "";
}

}  // namespace

namespace chatterino::commands {

QString deleteAllMessages(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /clear command only works in Twitch channels.");
        return "";
    }

    return deleteMessages(ctx.twitchChannel, QString());
}

QString deleteOneMessage(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    // This is a wrapper over the Helix delete messages endpoint
    // We use this to ensure the user gets better error messages for missing or malformed arguments
    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /delete command only works in Twitch channels.");
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage("Usage: /delete <msg-id> - Deletes the "
                                      "specified message.");
        return "";
    }

    auto messageID = ctx.words.at(1);
    auto uuid = QUuid(messageID);
    if (uuid.isNull())
    {
        // The message id must be a valid UUID
        ctx.channel->addSystemMessage(
            QString("Invalid msg-id: \"%1\"").arg(messageID));
        return "";
    }

    auto msg = ctx.channel->findMessage(messageID);
    if (msg != nullptr)
    {
        if (msg->loginName == ctx.channel->getName() &&
            !ctx.channel->isBroadcaster())
        {
            ctx.channel->addSystemMessage(
                "You cannot delete the broadcaster's messages unless "
                "you are the broadcaster.");
            return "";
        }
    }

    return deleteMessages(ctx.twitchChannel, messageID);
}

}  // namespace chatterino::commands
