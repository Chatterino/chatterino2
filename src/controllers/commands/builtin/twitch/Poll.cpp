#include "controllers/commands/builtin/twitch/Poll.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/common/ChannelAction.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace {

using namespace chatterino;

constexpr int MIN_POLL_DURATION = 15;    // seconds
constexpr int MAX_POLL_DURATION = 1800;  // seconds
constexpr int MAX_POLL_TITLE_LENGTH = 60;
constexpr int MAX_POLL_CHOICES = 5;

}  // namespace

namespace chatterino::commands {

QString createPoll(const CommandContext &ctx)
{
    const auto command = QStringLiteral("/poll");
    const auto usage = QStringLiteral(
        R"(Usage: "/poll --title "<title>" --duration <duration>[time unit] --choice "<choice1>" --choice "<choice2>" [options...]" - Creates a poll for users to vote among the defined options. Title may not exceed 60 characters. There must be between two and five poll choices. Duration must be a positive integer; time unit (optional, default=s) must be one of s, m; maximum duration is 30 minutes. Options: --points <points> to allow spending the specified channel points for each additional vote.)");
    const auto action = parseUserParticipationAction(
        ctx, command, usage, MIN_POLL_DURATION, MAX_POLL_DURATION,
        MAX_POLL_TITLE_LENGTH, MAX_POLL_CHOICES);

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
            "You must be logged in to create a poll!");
        return "";
    }

    if (currentUser->getUserId() != ctx.twitchChannel->roomId())
    {
        ctx.channel->addSystemMessage("Only the broadcaster can create polls "
                                      "due to Twitch restrictions.");
        return "";
    }

    auto poll = action.value();
    getHelix()->createPoll(
        poll.broadcasterId, poll.title, poll.choices, poll.duration,
        poll.pointsPerVote,
        [channel = ctx.channel, poll] {
            channel->addSystemMessage(
                QString("Created poll: '%1'").arg(poll.title));
        },
        [channel = ctx.channel](const auto &error) {
            channel->addSystemMessage("Failed to create poll - " + error);
        });

    return "";
}

}  // namespace chatterino::commands
