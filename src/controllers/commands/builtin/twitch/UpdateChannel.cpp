#include "controllers/commands/builtin/twitch/UpdateChannel.hpp"

#include "common/network/NetworkResult.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace {

using namespace chatterino;

QString formatUpdateChannelError(const char *updateType,
                                 HelixUpdateChannelError error,
                                 const QString &message)
{
    using Error = HelixUpdateChannelError;

    QString errorMessage = QString("Failed to set %1 - ").arg(updateType);

    switch (error)
    {
        case Error::UserMissingScope: {
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            errorMessage += QString("You must be the broadcaster "
                                    "to set the %1.")
                                .arg(updateType);
        }
        break;

        case Error::Ratelimited: {
            errorMessage += "You are being ratelimited by Twitch. Try "
                            "again in a few seconds.";
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

QString setTitle(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage("Usage: /settitle <stream title>");
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "Unable to set title of non-Twitch channel.");
        return "";
    }

    auto title = ctx.words.mid(1).join(" ");

    getHelix()->updateChannel(
        ctx.twitchChannel->roomId(), "", "", title,
        [channel{ctx.channel}, title](const auto &result) {
            (void)result;

            channel->addSystemMessage(
                QString("Updated title to %1").arg(title));
        },
        [channel{ctx.channel}](auto error, auto message) {
            auto errorMessage =
                formatUpdateChannelError("title", error, message);
            channel->addSystemMessage(errorMessage);
        });

    return "";
}

QString setGame(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage("Usage: /setgame <stream game>");
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "Unable to set game of non-Twitch channel.");
        return "";
    }

    const auto gameName = ctx.words.mid(1).join(" ");

    getHelix()->searchGames(
        gameName,
        [channel{ctx.channel}, twitchChannel{ctx.twitchChannel},
         gameName](const std::vector<HelixGame> &games) {
            if (games.empty())
            {
                channel->addSystemMessage("Game not found.");
                return;
            }

            auto matchedGame = games.at(0);

            if (games.size() > 1)
            {
                // NOTE: Improvements could be made with 'fuzzy string matching' code here
                // attempt to find the best looking game by comparing exactly with lowercase values
                for (const auto &game : games)
                {
                    if (game.name.toLower() == gameName.toLower())
                    {
                        matchedGame = game;
                        break;
                    }
                }
            }

            auto status = twitchChannel->accessStreamStatus();
            getHelix()->updateChannel(
                twitchChannel->roomId(), matchedGame.id, "", "",
                [channel, games, matchedGame](const NetworkResult &) {
                    channel->addSystemMessage(
                        QString("Updated game to %1").arg(matchedGame.name));
                },
                [channel](auto error, auto message) {
                    auto errorMessage =
                        formatUpdateChannelError("game", error, message);
                    channel->addSystemMessage(errorMessage);
                });
        },
        [channel{ctx.channel}] {
            channel->addSystemMessage("Failed to look up game.");
        });

    return "";
}

}  // namespace chatterino::commands
