#include "controllers/commands/builtin/twitch/Block.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "util/Twitch.hpp"

namespace {

using namespace chatterino;

}  // namespace

namespace chatterino::commands {

QString blockUser(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /block command only works in Twitch channels.");
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage("Usage: /block <user>");
        return "";
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();

    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to block someone!");
        return "";
    }

    auto target = ctx.words.at(1);
    stripChannelName(target);

    getHelix()->getUserByName(
        target,
        [currentUser, channel{ctx.channel},
         target](const HelixUser &targetUser) {
            getApp()->getAccounts()->twitch.getCurrent()->blockUser(
                targetUser.id, nullptr,
                [channel, target, targetUser] {
                    channel->addSystemMessage(
                        QString("You successfully blocked user %1")
                            .arg(target));
                },
                [channel, target] {
                    channel->addSystemMessage(
                        QString("User %1 couldn't be blocked, an unknown "
                                "error occurred!")
                            .arg(target));
                });
        },
        [channel{ctx.channel}, target] {
            channel->addSystemMessage(QString("User %1 couldn't be blocked, no "
                                              "user with that name found!")
                                          .arg(target));
        });

    return "";
}

QString ignoreUser(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    ctx.channel->addSystemMessage(
        "Ignore command has been renamed to /block, please use it from "
        "now on as /ignore is going to be removed soon.");

    return blockUser(ctx);
}

QString unblockUser(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    if (ctx.twitchChannel == nullptr)
    {
        ctx.channel->addSystemMessage(
            "The /unblock command only works in Twitch channels.");
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage("Usage: /unblock <user>");
        return "";
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();

    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            "You must be logged in to unblock someone!");
        return "";
    }

    auto target = ctx.words.at(1);
    stripChannelName(target);

    getHelix()->getUserByName(
        target,
        [currentUser, channel{ctx.channel}, target](const auto &targetUser) {
            getApp()->getAccounts()->twitch.getCurrent()->unblockUser(
                targetUser.id, nullptr,
                [channel, target, targetUser] {
                    channel->addSystemMessage(
                        QString("You successfully unblocked user %1")
                            .arg(target));
                },
                [channel, target] {
                    channel->addSystemMessage(
                        QString("User %1 couldn't be unblocked, an unknown "
                                "error occurred!")
                            .arg(target));
                });
        },
        [channel{ctx.channel}, target] {
            channel->addSystemMessage(QString("User %1 couldn't be unblocked, "
                                              "no user with that name found!")
                                          .arg(target));
        });

    return "";
}

QString unignoreUser(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    ctx.channel->addSystemMessage(
        "Unignore command has been renamed to /unblock, please use it "
        "from now on as /unignore is going to be removed soon.");
    return unblockUser(ctx);
}

}  // namespace chatterino::commands
