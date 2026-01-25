// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/commands/builtin/twitch/LowTrust.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace {

using namespace chatterino;

void addSuspiciousTreatment(const CommandContext &ctx, const QString &command,
                            const QString &usage, bool restrict)
{
    if (ctx.twitchChannel == nullptr)
    {
        // This action must be performed with a twitch channel as a context
        const QString error =
            "The " % command % " command only works in Twitch channels";
        if (ctx.channel != nullptr)
        {
            ctx.channel->addSystemMessage(error);
        }
        else
        {
            qCWarning(chatterinoCommands) << "Error parsing command:" << error;
        }
        return;
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage(usage);
        return;
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage("You must be logged in to " % command %
                                      " someone!");
        return;
    }

    auto roomId = ctx.twitchChannel->roomId();
    auto modId = currentUser->getUserId();
    getHelix()->getUserByName(
        ctx.words.at(1),
        [chan{ctx.channel}, roomId, modId, command, restrict](const auto &u) {
            getHelix()->addSuspiciousUser(
                roomId, modId, u.id, restrict,
                [] {
                    // treatment notification is handled by eventsub
                },
                [chan, command](const auto &err) {
                    chan->addSystemMessage("Failed to " % command % " user - " %
                                           err);
                });
        },
        [chan = ctx.channel, command] {
            chan->addSystemMessage("Failed to query user to " % command);
        });
}

void removeSuspiciousTreatment(const CommandContext &ctx,
                               const QString &command, const QString &usage)
{
    if (ctx.twitchChannel == nullptr)
    {
        // This action must be performed with a twitch channel as a context
        const QString error =
            "The " % command % " command only works in Twitch channels";
        if (ctx.channel != nullptr)
        {
            ctx.channel->addSystemMessage(error);
        }
        else
        {
            qCWarning(chatterinoCommands) << "Error parsing command:" << error;
        }
        return;
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage(usage);
        return;
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage("You must be logged in to " % command %
                                      " someone!");
        return;
    }

    auto roomId = ctx.twitchChannel->roomId();
    auto modId = currentUser->getUserId();
    getHelix()->getUserByName(
        ctx.words.at(1),
        [chan{ctx.channel}, roomId, modId, command](const auto &user) {
            getHelix()->removeSuspiciousUser(
                roomId, modId, user.id,
                [] {
                    // treatment notification is handled by eventsub
                },
                [chan, command](const auto &err) {
                    chan->addSystemMessage("Failed to " % command % " user - " %
                                           err);
                });
        },
        [chan = ctx.channel, command] {
            chan->addSystemMessage("Failed to query user to " % command);
        });
}

}  // namespace

namespace chatterino::commands {

QString monitorUser(const CommandContext &ctx)
{
    const auto command = QStringLiteral("/monitor");
    const auto usage = QStringLiteral(
        R"(Usage: "/monitor <username>" - Mark a user as monitored.)");

    addSuspiciousTreatment(ctx, command, usage, false);

    return "";
}

QString restrictUser(const CommandContext &ctx)
{
    const auto command = QStringLiteral("/restrict");
    const auto usage = QStringLiteral(
        R"(Usage: "/restrict <username>" - Mark a user as restricted.)");

    addSuspiciousTreatment(ctx, command, usage, true);

    return "";
}

QString unmonitorUser(const CommandContext &ctx)
{
    const auto command = QStringLiteral("/unmonitor");
    const auto usage = QStringLiteral(
        R"(Usage: "/unmonitor <username>" - Remove a user from suspicious treatment.)");

    removeSuspiciousTreatment(ctx, command, usage);

    return "";
}

QString unrestrictUser(const CommandContext &ctx)
{
    const auto command = QStringLiteral("/unrestrict");
    const auto usage = QStringLiteral(
        R"(Usage: "/unrestrict <username>" - Remove a user from suspicious treatment.)");

    removeSuspiciousTreatment(ctx, command, usage);

    return "";
}

}  // namespace chatterino::commands
