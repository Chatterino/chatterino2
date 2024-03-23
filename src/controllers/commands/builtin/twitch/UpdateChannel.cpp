#include "controllers/commands/builtin/twitch/UpdateChannel.hpp"

#include "common/Channel.hpp"
#include "common/network/NetworkResult.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino::commands {

QString setTitle(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(
            makeSystemMessage("Usage: /settitle <stream title>"));
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(
            makeSystemMessage("Unable to set title of non-Twitch channel."));
        return "";
    }

    auto status = ctx.twitchChannel->accessStreamStatus();
    auto title = ctx.words.mid(1).join(" ");
    getHelix()->updateChannel(
        ctx.twitchChannel->roomId(), "", "", title,
        [channel{ctx.channel}, title](const auto &result) {
            (void)result;

            channel->addMessage(
                makeSystemMessage(QString("Updated title to %1").arg(title)));
        },
        [channel{ctx.channel}] {
            channel->addMessage(
                makeSystemMessage("Title update failed! Are you "
                                  "missing the required scope?"));
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
        ctx.channel->addMessage(
            makeSystemMessage("Usage: /setgame <stream game>"));
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addMessage(
            makeSystemMessage("Unable to set game of non-Twitch channel."));
        return "";
    }

    const auto gameName = ctx.words.mid(1).join(" ");

    getHelix()->searchGames(
        gameName,
        [channel{ctx.channel}, twitchChannel{ctx.twitchChannel},
         gameName](const std::vector<HelixGame> &games) {
            if (games.empty())
            {
                channel->addMessage(makeSystemMessage("Game not found."));
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
                    channel->addMessage(makeSystemMessage(
                        QString("Updated game to %1").arg(matchedGame.name)));
                },
                [channel] {
                    channel->addMessage(
                        makeSystemMessage("Game update failed! Are you "
                                          "missing the required scope?"));
                });
        },
        [channel{ctx.channel}] {
            channel->addMessage(makeSystemMessage("Failed to look up game."));
        });

    return "";
}

}  // namespace chatterino::commands
