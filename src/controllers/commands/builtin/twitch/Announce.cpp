#include "controllers/commands/builtin/twitch/Announce.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino::commands {

QString sendAnnouncement(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "This command can only be used in Twitch channels."));
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "Usage: /announce <message> - Call attention to your "
            "message with a highlight."));
        return "";
    }

    auto user = getApp()->accounts->twitch.getCurrent();
    if (user->isAnon())
    {
        ctx.channel->addMessage(makeSystemMessage(
            "You must be logged in to use the /announce command."));
        return "";
    }

    getHelix()->sendChatAnnouncement(
        ctx.twitchChannel->roomId(), user->getUserId(),
        ctx.words.mid(1).join(" "), HelixAnnouncementColor::Primary,
        []() {
            // do nothing.
        },
        [channel{ctx.channel}](auto error, auto message) {
            using Error = HelixSendChatAnnouncementError;
            QString errorMessage = QString("Failed to send announcement - ");

            switch (error)
            {
                case Error::UserMissingScope: {
                    // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                    errorMessage +=
                        "Missing required scope. Re-login with your "
                        "account and try again.";
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
    return "";
}

}  // namespace chatterino::commands
