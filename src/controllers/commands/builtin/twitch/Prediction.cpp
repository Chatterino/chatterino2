#include "controllers/commands/builtin/twitch/Prediction.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/common/ChannelAction.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

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

}  // namespace chatterino::commands
