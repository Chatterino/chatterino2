#include "controllers/commands/builtin/twitch/Chatters.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Env.hpp"
#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Theme.hpp"

#include <QApplication>
#include <QLoggingCategory>
#include <QString>

namespace {

using namespace chatterino;

QString formatChattersError(HelixGetChattersError error, const QString &message)
{
    using Error = HelixGetChattersError;

    QString errorMessage = QString("Failed to get chatter count - ");

    switch (error)
    {
        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::UserMissingScope: {
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            errorMessage += "You must have moderator permissions to "
                            "use this command.";
        }
        break;

        case Error::Unknown: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

}  // namespace

namespace chatterino::commands {

QString chatters(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /chatters command only works in Twitch Channels.");
        return "";
    }

    // Refresh chatter list via helix api for mods
    getHelix()->getChatters(
        ctx.twitchChannel->roomId(),
        getApp()->getAccounts()->twitch.getCurrent()->getUserId(), 1,
        [channel{ctx.channel}](auto result) {
            channel->addSystemMessage(QString("Chatter count: %1.")
                                          .arg(localizeNumbers(result.total)));
        },
        [channel{ctx.channel}](auto error, auto message) {
            auto errorMessage = formatChattersError(error, message);
            channel->addSystemMessage(errorMessage);
        });

    return "";
}

QString testChatters(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /test-chatters command only works in Twitch Channels.");
        return "";
    }

    getHelix()->getChatters(
        ctx.twitchChannel->roomId(),
        getApp()->getAccounts()->twitch.getCurrent()->getUserId(), 5000,
        [channel{ctx.channel}, twitchChannel{ctx.twitchChannel}](auto result) {
            QStringList entries;
            for (const auto &username : result.chatters)
            {
                entries << username;
            }

            QString prefix = "Chatters ";

            if (result.total > 5000)
            {
                prefix += QString("(5000/%1):").arg(result.total);
            }
            else
            {
                prefix += QString("(%1):").arg(result.total);
            }

            channel->addMessage(MessageBuilder::makeListOfUsersMessage(
                                    prefix, entries, twitchChannel),
                                MessageContext::Original);
        },
        [channel{ctx.channel}](auto error, auto message) {
            auto errorMessage = formatChattersError(error, message);
            channel->addSystemMessage(errorMessage);
        });

    return "";
}

}  // namespace chatterino::commands
