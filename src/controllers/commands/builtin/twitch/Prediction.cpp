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

#include <QCommandLineParser>
#include <QProcess>

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

QString completePrediction(const CommandContext &ctx)
{
    const auto usage = QStringLiteral(
        R"(Usage: /completeprediction --choice "<choice>" or /completeprediction --index <index> - Selects a winner for an outstanding prediction. The choice title must exactly match the wording in the prediction. Alternatively, you may specify the one-based index of the winning outcome.)");

    if (ctx.twitchChannel == nullptr)
    {
        const auto err = QStringLiteral(
            "The /completeprediction command only works in Twitch channels");
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

    // Define arguments
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(
        QCommandLineParser::ParseAsOptions);
    QCommandLineOption choiceOption(
        {"c", "choice"}, "The prediction outcome to select as the winner",
        "choice");
    QCommandLineOption indexOption(
        {"i", "index"},
        "The one-based index of the prediction outcome to select as the winner",
        "index");
    parser.addOptions({
        choiceOption,
        indexOption,
    });
    const auto joined = ctx.words.join(" ");
    parser.parse(QProcess::splitCommand(joined));

    // Input validation
    const bool hasName = parser.isSet(choiceOption);
    const bool hasIndex = parser.isSet(indexOption);
    if (hasName && hasIndex)
    {
        ctx.channel->addSystemMessage(
            "You may not specify choice and index simultaneously - " + usage);
        return "";
    }
    if (!hasIndex && !hasName)
    {
        ctx.channel->addSystemMessage(
            "You must specify either choice or index - " + usage);
        return "";
    }

    int targetIndex = 0;
    QString targetName;
    if (hasName)
    {
        targetName = parser.value(choiceOption);
    }
    else
    {
        bool ok = true;
        targetIndex = parser.value(indexOption).toInt(&ok);
        if (!ok || targetIndex <= 0)
        {
            ctx.channel->addSystemMessage("Invalid index - " + usage);
            return "";
        }
    }

    // Perform action
    const auto roomId = ctx.twitchChannel->roomId();
    getHelix()->getPredictions(
        roomId, {}, 1, {},
        [channel = ctx.channel, roomId, hasIndex, targetIndex,
         targetName](const auto &queryResult) {
            if (queryResult.predictions.empty())
            {
                channel->addSystemMessage(
                    "You must start a prediction before you can complete one");
                return;
            }

            auto prediction = queryResult.predictions.front();
            if (prediction.status != "ACTIVE" && prediction.status != "LOCKED")
            {
                channel->addSystemMessage(
                    "Could not find an open prediction to complete");
                return;
            }

            // identify winning outcome
            auto outcomes = prediction.outcomes;
            QString winnerId = "";
            if (hasIndex)
            {
                auto maxIndex = outcomes.size();
                if (targetIndex > maxIndex)
                {
                    channel->addSystemMessage(
                        QString("Specified index (%1) exceeds the number of "
                                "outcomes (%2)")
                            .arg(QString::number(targetIndex))
                            .arg(QString::number(maxIndex)));
                    return;
                }

                winnerId = outcomes[targetIndex - 1].id;
            }
            else
            {
                for (const auto &outcome : outcomes)
                {
                    if (outcome.title == targetName)
                    {
                        winnerId = outcome.id;
                        break;
                    }
                }

                if (winnerId == "")
                {
                    auto options = std::accumulate(
                        outcomes.begin(), outcomes.end(), QString{},
                        [](const QString &acc,
                           const HelixPredictionOutcome &outcome) {
                            auto title = "'" + outcome.title + "'";
                            return acc.isEmpty() ? title : acc + ", " + title;
                        });
                    channel->addSystemMessage(
                        "Could not find the desired winner. Options include: " +
                        options);
                    return;
                }
            }

            // resolve prediction
            getHelix()->endPrediction(
                roomId, prediction.id, false, winnerId,
                [channel](const HelixPrediction &result) {
                    int totalPoints = 0;
                    HelixPredictionOutcome winner = result.outcomes.front();
                    for (const auto &outcome : result.outcomes)
                    {
                        totalPoints += outcome.channelPoints;
                        if (outcome.id == result.winningOutcomeID)
                        {
                            winner = outcome;
                        }
                    }

                    channel->addSystemMessage(
                        QString("Completed prediction: %1 - '%2' won %3 points "
                                "(%4 profit) to be distributed among %5 users")
                            .arg(result.title, winner.title,
                                 localizeNumbers(totalPoints),
                                 localizeNumbers(totalPoints -
                                                 winner.channelPoints),
                                 localizeNumbers(winner.users)));
                },
                [channel](const auto &error) {
                    channel->addSystemMessage(
                        "Failed to complete prediction - " + error);
                });
        },
        [channel = ctx.channel](const auto &error) {
            channel->addSystemMessage(
                "Failed to query predictions to complete - " + error);
        });

    return "";
}

}  // namespace chatterino::commands
