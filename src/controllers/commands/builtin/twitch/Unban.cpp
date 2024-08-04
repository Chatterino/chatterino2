#include "controllers/commands/builtin/twitch/Unban.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/common/ChannelAction.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"

namespace {

using namespace chatterino;

void unbanUserByID(const ChannelPtr &channel, const QString &channelID,
                   const QString &sourceUserID, const QString &targetUserID,
                   const QString &displayName)
{
    getHelix()->unbanUser(
        channelID, sourceUserID, targetUserID,
        [] {
            // No response for unbans, they're emitted over pubsub/IRC instead
        },
        [channel, displayName](auto error, auto message) {
            using Error = HelixUnbanUserError;

            QString errorMessage = QString("Failed to unban user - ");

            switch (error)
            {
                case Error::ConflictingOperation: {
                    errorMessage += "There was a conflicting ban operation on "
                                    "this user. Please try again.";
                }
                break;

                case Error::Forwarded: {
                    errorMessage += message;
                }
                break;

                case Error::Ratelimited: {
                    errorMessage += "You are being ratelimited by Twitch. Try "
                                    "again in a few seconds.";
                }
                break;

                case Error::TargetNotBanned: {
                    // Equivalent IRC error
                    errorMessage =
                        QString("%1 is not banned from this channel.")
                            .arg(displayName);
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

            channel->addSystemMessage(errorMessage);
        });
}

}  // namespace

namespace chatterino::commands {

QString unbanUser(const CommandContext &ctx)
{
    const auto command = ctx.words.at(0).toLower();
    const auto usage =
        QStringLiteral(
            R"(Usage: "%1 <username> - Removes a ban on a user. Options: --channel <channel> to override which channel the unban takes place in (can be specified multiple times).)")
            .arg(command);
    const auto actions = parseChannelAction(ctx, command, usage, false, false);
    if (!actions.has_value())
    {
        if (ctx.channel != nullptr)
        {
            ctx.channel->addSystemMessage(actions.error());
        }
        else
        {
            qCWarning(chatterinoCommands)
                << "Error parsing command:" << actions.error();
        }

        return "";
    }

    assert(!actions.value().empty());

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to unban someone!");
        return "";
    }

    for (const auto &action : actions.value())
    {
        const auto &reason = action.reason;

        QStringList userLoginsToFetch;
        QStringList userIDs;
        if (action.target.id.isEmpty())
        {
            assert(!action.target.login.isEmpty() &&
                   "Unban Action target username AND user ID may not be "
                   "empty at the same time");
            userLoginsToFetch.append(action.target.login);
        }
        else
        {
            // For hydration
            userIDs.append(action.target.id);
        }
        if (action.channel.id.isEmpty())
        {
            assert(!action.channel.login.isEmpty() &&
                   "Unban Action channel username AND user ID may not be "
                   "empty at the same time");
            userLoginsToFetch.append(action.channel.login);
        }
        else
        {
            // For hydration
            userIDs.append(action.channel.id);
        }

        if (!userLoginsToFetch.isEmpty())
        {
            // At least 1 user ID needs to be resolved before we can take action
            // userIDs is filled up with the data we already have to hydrate the action channel & action target
            getHelix()->fetchUsers(
                userIDs, userLoginsToFetch,
                [channel{ctx.channel}, actionChannel{action.channel},
                 actionTarget{action.target}, currentUser, reason,
                 userLoginsToFetch](const auto &users) mutable {
                    if (!actionChannel.hydrateFrom(users))
                    {
                        channel->addSystemMessage(
                            QString("Failed to timeout, bad channel name: %1")
                                .arg(actionChannel.login));
                        return;
                    }
                    if (!actionTarget.hydrateFrom(users))
                    {
                        channel->addSystemMessage(
                            QString("Failed to timeout, bad target name: %1")
                                .arg(actionTarget.login));
                        return;
                    }

                    unbanUserByID(channel, actionChannel.id,
                                  currentUser->getUserId(), actionTarget.id,
                                  actionTarget.displayName);
                },
                [channel{ctx.channel}, userLoginsToFetch] {
                    channel->addSystemMessage(
                        QString("Failed to timeout, bad username(s): %1")
                            .arg(userLoginsToFetch.join(", ")));
                });
        }
        else
        {
            // If both IDs are available, we do no hydration & just use the id as the display name
            unbanUserByID(ctx.channel, action.channel.id,
                          currentUser->getUserId(), action.target.id,
                          action.target.id);
        }
    }

    return "";
}

}  // namespace chatterino::commands
