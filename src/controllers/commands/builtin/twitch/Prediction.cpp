#include "controllers/commands/builtin/twitch/Prediction.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/common/ChannelAction.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Helpers.hpp"

#include <chrono>

namespace {

using namespace chatterino;

constexpr auto MIN_PREDICT_DURATION = std::chrono::seconds(30);
constexpr auto MAX_PREDICT_DURATION = std::chrono::seconds(1800);

}  // namespace

namespace chatterino::commands {

QString createPrediction(const CommandContext &ctx)
{
    const auto command = QStringLiteral("/prediction");
    const auto usage = QStringLiteral(
        R"(Usage: "/prediction --title "<title>" --choice "<choice1>" --choice "<choice2>" --duration <duration>[time unit]" - Creates a prediction for users to guess among the defined options. Title may not exceed 45 characters. There must be between two and ten choices. Duration must be a positive integer; time unit (optional, default=s) must be one of s, m; maximum duration is 30 minutes.)");
    const auto action = parseUserParticipationAction(
        ctx, command, usage, MIN_PREDICT_DURATION, MAX_PREDICT_DURATION);

    if (!action.has_value())
    {
        if (ctx.channel != nullptr)
        {
            ctx.channel->addSystemMessage(action.error());
        }
        else
        {
            qCWarning(chatterinoCommands)
                << "Error parsing command:" << action.error();
        }
        return "";
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to create a prediction!");
        return "";
    }

    const auto &data = action.value();
    getHelix()->createPrediction(
        data.broadcasterID, data.title, data.choices, data.duration,
        [channel = ctx.channel, data] {
            channel->addSystemMessage(
                QString("Created prediction: '%1'").arg(data.title));
        },
        [channel = ctx.channel](const auto &error) {
            channel->addSystemMessage("Failed to create prediction - " + error);
        });

    return "";
}

QString lockPrediction(const CommandContext &ctx)
{
    if (ctx.twitchChannel == nullptr)
    {
        const auto err = QStringLiteral(
            "The /lockprediction command only works in Twitch channels");
        if (ctx.channel != nullptr)
        {
            ctx.channel->addSystemMessage(err);
        }
        else
        {
            qCWarning(chatterinoCommands) << "Invalid command context:" << err;
        }
        return "";
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to lock a prediction!");
        return "";
    }

    const auto roomId = ctx.twitchChannel->roomId();
    getHelix()->getPredictions(
        roomId, {}, 1, {},
        [channel = ctx.channel, roomId](const auto &result) {
            if (result.predictions.empty())
            {
                channel->addSystemMessage("Failed to find any predictions");
                return;
            }

            auto prediction = result.predictions.front();
            if (prediction.status == "LOCKED")
            {
                channel->addSystemMessage(
                    "The current prediction is already locked: " +
                    prediction.title);
                return;
            }

            if (prediction.status != "ACTIVE")
            {
                channel->addSystemMessage(
                    "Could not find an active prediction");
                return;
            }

            getHelix()->endPrediction(
                roomId, prediction.id, false, {},
                [channel](const HelixPrediction &data) {
                    int totalPoints = 0;
                    int numUsers = 0;
                    for (const auto &outcome : data.outcomes)
                    {
                        totalPoints += outcome.channelPoints;
                        numUsers += outcome.users;
                    }

                    channel->addSystemMessage(
                        QString("Locked prediction with %1 points wagered by "
                                "%2 users: '%3'")
                            .arg(localizeNumbers(totalPoints),
                                 localizeNumbers(numUsers), data.title));
                },
                [channel](const auto &error) {
                    channel->addSystemMessage("Failed to lock prediction - " +
                                              error);
                });
        },
        [channel = ctx.channel](const auto &error) {
            channel->addSystemMessage("Failed to query predictions - " + error);
        });

    return "";
}

QString cancelPrediction(const CommandContext &ctx)
{
    if (ctx.twitchChannel == nullptr)
    {
        const auto err = QStringLiteral(
            "The /cancelprediction command only works in Twitch channels");
        if (ctx.channel != nullptr)
        {
            ctx.channel->addSystemMessage(err);
        }
        else
        {
            qCWarning(chatterinoCommands) << "Invalid command context:" << err;
        }
        return "";
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to cancel a prediction!");
        return "";
    }

    const auto roomId = ctx.twitchChannel->roomId();
    getHelix()->getPredictions(
        roomId, {}, 1, {},
        [channel = ctx.channel, roomId](const auto &result) {
            if (result.predictions.empty())
            {
                channel->addSystemMessage("Failed to find any predictions");
                return;
            }

            auto prediction = result.predictions.front();
            if (prediction.status != "ACTIVE" && prediction.status != "LOCKED")
            {
                channel->addSystemMessage("Could not find an open prediction");
                return;
            }

            getHelix()->endPrediction(
                roomId, prediction.id, true, {},
                [channel](const HelixPrediction &data) {
                    int totalPoints = 0;
                    int numUsers = 0;
                    for (const auto &outcome : data.outcomes)
                    {
                        totalPoints += outcome.channelPoints;
                        numUsers += outcome.users;
                    }

                    channel->addSystemMessage(
                        QString("Refunded %1 points to %2 users for "
                                "prediction: '%3'")
                            .arg(localizeNumbers(totalPoints),
                                 localizeNumbers(numUsers), data.title));
                },
                [channel](const auto &error) {
                    channel->addSystemMessage("Failed to cancel prediction - " +
                                              error);
                });
        },
        [channel = ctx.channel](const auto &error) {
            channel->addSystemMessage("Failed to query predictions - " + error);
        });

    return "";
}

}  // namespace chatterino::commands
