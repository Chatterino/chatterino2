#include "controllers/commands/builtin/twitch/Block.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
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
        ctx.channel->addMessage(makeSystemMessage(
            "The /block command only works in Twitch channels."));
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage("Usage: /block <user>"));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();

    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to block someone!"));
        return "";
    }

    auto target = ctx.words.at(1);
    stripChannelName(target);

    getHelix()->getUserByName(
        target,
        [currentUser, channel{ctx.channel},
         target](const HelixUser &targetUser) {
            getApp()->accounts->twitch.getCurrent()->blockUser(
                targetUser.id, nullptr,
                [channel, target, targetUser] {
                    channel->addMessage(makeSystemMessage(
                        QString("You successfully blocked user %1")
                            .arg(target)));
                },
                [channel, target] {
                    channel->addMessage(makeSystemMessage(
                        QString("User %1 couldn't be blocked, an unknown "
                                "error occurred!")
                            .arg(target)));
                });
        },
        [channel{ctx.channel}, target] {
            channel->addMessage(
                makeSystemMessage(QString("User %1 couldn't be blocked, no "
                                          "user with that name found!")
                                      .arg(target)));
        });

    return "";
}

QString ignoreUser(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    ctx.channel->addMessage(makeSystemMessage(
        "Ignore command has been renamed to /block, please use it from "
        "now on as /ignore is going to be removed soon."));

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
        ctx.channel->addMessage(makeSystemMessage(
            "The /unblock command only works in Twitch channels."));
        return "";
    }

    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage("Usage: /unblock <user>"));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();

    if (currentUser->isAnon())
    {
        ctx.channel->addMessage(
            makeSystemMessage("You must be logged in to unblock someone!"));
        return "";
    }

    auto target = ctx.words.at(1);
    stripChannelName(target);

    getHelix()->getUserByName(
        target,
        [currentUser, channel{ctx.channel}, target](const auto &targetUser) {
            getApp()->accounts->twitch.getCurrent()->unblockUser(
                targetUser.id, nullptr,
                [channel, target, targetUser] {
                    channel->addMessage(makeSystemMessage(
                        QString("You successfully unblocked user %1")
                            .arg(target)));
                },
                [channel, target] {
                    channel->addMessage(makeSystemMessage(
                        QString("User %1 couldn't be unblocked, an unknown "
                                "error occurred!")
                            .arg(target)));
                });
        },
        [channel{ctx.channel}, target] {
            channel->addMessage(
                makeSystemMessage(QString("User %1 couldn't be unblocked, "
                                          "no user with that name found!")
                                      .arg(target)));
        });

    return "";
}

QString unignoreUser(const CommandContext &ctx)
{
    if (ctx.channel == nullptr)
    {
        return "";
    }

    ctx.channel->addMessage(makeSystemMessage(
        "Unignore command has been renamed to /unblock, please use it "
        "from now on as /unignore is going to be removed soon."));
    return unblockUser(ctx);
}

}  // namespace chatterino::commands
