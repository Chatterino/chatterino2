#include "controllers/commands/common/ChannelAction.hpp"

#include "controllers/commands/CommandContext.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Helpers.hpp"
#include "util/Twitch.hpp"

#include <QCommandLineParser>
#include <QStringBuilder>

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

namespace chatterino::commands {

bool IncompleteHelixUser::hydrateFrom(const std::vector<HelixUser> &users)
{
    // Find user in list based on our id or login
    auto resolvedIt =
        std::find_if(users.begin(), users.end(), [this](const auto &user) {
            if (!this->login.isEmpty())
            {
                return user.login.compare(this->login, Qt::CaseInsensitive) ==
                       0;
            }
            if (!this->id.isEmpty())
            {
                return user.id.compare(this->id, Qt::CaseInsensitive) == 0;
            }
            return false;
        });
    if (resolvedIt == users.end())
    {
        return false;
    }
    const auto &resolved = *resolvedIt;
    this->id = resolved.id;
    this->login = resolved.login;
    this->displayName = resolved.displayName;
    return true;
}

std::ostream &operator<<(std::ostream &os, const IncompleteHelixUser &u)
{
    os << "{id:" << u.id.toStdString() << ", login:" << u.login.toStdString()
       << ", displayName:" << u.displayName.toStdString() << '}';
    return os;
}

void PrintTo(const PerformChannelAction &a, std::ostream *os)
{
    *os << "{channel:" << a.channel << ", target:" << a.target
        << ", reason:" << a.reason.toStdString()
        << ", duration:" << std::to_string(a.duration) << '}';
}

nonstd::expected<std::vector<PerformChannelAction>, QString> parseChannelAction(
    const CommandContext &ctx, const QString &command, const QString &usage,
    bool withDuration, bool withReason)
{
    if (ctx.channel == nullptr)
    {
        // A ban action must be performed with a channel as a context
        return nonstd::make_unexpected(
            "A " % command %
            " action must be performed with a channel as a context");
    }

    QCommandLineParser parser;
    parser.setOptionsAfterPositionalArgumentsMode(
        QCommandLineParser::ParseAsPositionalArguments);
    parser.addPositionalArgument("username", "The name of the user to ban");
    if (withDuration)
    {
        parser.addPositionalArgument("duration", "Duration of the action");
    }
    if (withReason)
    {
        parser.addPositionalArgument("reason", "The optional ban reason");
    }
    QCommandLineOption channelOption(
        "channel", "Override which channel(s) to perform the action in",
        "channel");
    parser.addOptions({
        channelOption,
    });
    parser.parse(ctx.words);

    auto positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty())
    {
        return nonstd::make_unexpected("Missing target - " % usage);
    }

    auto [targetUserName, targetUserID] =
        parseUserNameOrID(positionalArguments.takeFirst());

    PerformChannelAction base{
        .target =
            IncompleteHelixUser{
                .id = targetUserID,
                .login = targetUserName,
                .displayName = "",
            },
        .duration = 0,
    };

    if (withDuration)
    {
        if (positionalArguments.isEmpty())
        {
            base.duration = 10 * 60;  // 10 min
        }
        else
        {
            auto durationStr = positionalArguments.takeFirst();
            base.duration = (int)parseDurationToSeconds(durationStr);
            if (base.duration <= 0)
            {
                return nonstd::make_unexpected("Invalid duration - " % usage);
            }
            if (withReason)
            {
                base.reason = positionalArguments.join(' ');
            }
        }
    }
    else
    {
        if (withReason)
        {
            base.reason = positionalArguments.join(' ');
        }
    }

    std::vector<PerformChannelAction> actions;

    auto overrideChannels = parser.values(channelOption);
    if (overrideChannels.isEmpty())
    {
        if (ctx.twitchChannel == nullptr)
        {
            return nonstd::make_unexpected(
                "The " % command % " command only works in Twitch channels");
        }

        actions.push_back(PerformChannelAction{
            .channel =
                {
                    .id = ctx.twitchChannel->roomId(),
                    .login = ctx.twitchChannel->getName(),
                    .displayName = ctx.twitchChannel->getDisplayName(),
                },
            .target = base.target,
            .reason = base.reason,
            .duration = base.duration,
        });
    }
    else
    {
        for (const auto &overrideChannelTarget : overrideChannels)
        {
            auto [channelUserName, channelUserID] =
                parseUserNameOrID(overrideChannelTarget);
            actions.push_back(PerformChannelAction{
                .channel =
                    {
                        .id = channelUserID,
                        .login = channelUserName,
                    },
                .target = base.target,
                .reason = base.reason,
                .duration = base.duration,
            });
        }
    }

    return actions;
}

}  // namespace chatterino::commands
