// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Expected.hpp"
#include "util/Helpers.hpp"

#include <controllers/commands/builtin/twitch/Pin.hpp>
#include <QCommandLineParser>
#include <QProcess>

namespace {

using namespace Qt::Literals;
using namespace chatterino;

struct Action {
    QString text;
    QString messageID;
    std::optional<std::chrono::seconds> duration;
};

// Duration of pin when sending message with "Send Chat Message".
// See https://dev.twitch.tv/docs/api/reference#send-chat-message.
constexpr std::chrono::minutes SEND_CHAT_MESSAGE_PIN_DURATION(20);

void sendPinnedMessage(const std::shared_ptr<TwitchChannel> &chan,
                       const std::shared_ptr<TwitchAccount> &moderator,
                       const QString &text,
                       std::optional<std::chrono::seconds> duration)
{
    getHelix()->sendChatMessage(
        {
            .broadcasterID = chan->roomId(),
            .senderID = moderator->getUserId(),
            .message = text,
            .pin = true,
        },
        [weak = std::weak_ptr(chan), origText = text, duration,
         moderator](const HelixSentMessage &res) {
            auto chan = weak.lock();
            if (!chan)
            {
                return;
            }

            if (!res.isSent)
            {
                if (res.dropReason)
                {
                    chan->addSystemMessage(res.dropReason->message);
                }
                else
                {
                    chan->addSystemMessage("Your message was not pinned.");
                }
                return;
            }

            // Default duration, no need to update it once more.
            if (duration == SEND_CHAT_MESSAGE_PIN_DURATION)
            {
                chan->addMessage(
                    MessageBuilder::makePinSuccessMessage(origText, res.id),
                    MessageContext::Original);
                return;
            }

            chan->updatePinnedMessageAs(res.id, duration, *moderator, origText);
        },
        [weak = std::weak_ptr(chan)](auto error, auto message) {
            auto chan = weak.lock();
            if (!chan)
            {
                return;
            }

            if (message.isEmpty())
            {
                message = "(empty message)";
            }

            using Error = HelixSendMessageError;

            auto errorMessage = [&]() -> QString {
                switch (error)
                {
                    case Error::MissingText:
                        return "You can't pin an empty message.";
                    case Error::BadRequest:
                        return "Failed to pin message: " + message;
                    case Error::Forbidden:
                        return "You are not allowed to pin messages in this "
                               "channel.";
                    case Error::MessageTooLarge:
                        return "Your message was too long.";
                    case Error::UserMissingScope:
                        return "Missing required scope. Re-login with your "
                               "account and try again.";
                    case Error::Forwarded:
                        return message;
                    case Error::Unknown:
                    default:
                        return "Unknown error: " + message;
                }
            }();
            chan->addSystemMessage(errorMessage);
        });
}

ExpectedStr<Action> parseAction(const CommandContext &ctx)
{
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(
        QCommandLineParser::ParseAsOptions);
    QCommandLineOption idOption(u"id"_s, {}, u"ID"_s);
    QCommandLineOption durationOption(QStringList{u"d"_s, u"duration"_s}, {},
                                      u"duration"_s);
    parser.addOptions({idOption, durationOption});
    parser.parse(QProcess::splitCommand(ctx.words.join(" ")));

    std::optional<std::chrono::seconds> duration;
    auto durationText = parser.value(durationOption);
    bool isExplicitUntilEnd =
        durationText == u"until-end" || durationText == u"none";
    if (!durationText.isEmpty() && !isExplicitUntilEnd)
    {
        const auto dur = parseDurationToSeconds(durationText);
        if (dur <= 0)
        {
            return makeUnexpected(u"Duration must be positive."_s);
        }
        duration.emplace(dur);
    }

    std::optional<QString> id;
    if (parser.isSet(idOption))
    {
        id.emplace(parser.value(idOption));
    }

    auto positionals = parser.positionalArguments().join(' ');

    if (id)
    {
        if (!positionals.isEmpty())
        {
            return makeUnexpected(
                u"No positional arguments can be specified when using --id."_s);
        }

        return Action{
            .messageID = *std::move(id),
            .duration = duration,
        };
    }

    if (!duration && !isExplicitUntilEnd)
    {
        duration = SEND_CHAT_MESSAGE_PIN_DURATION;
    }

    return Action{
        .text = positionals,
        .duration = duration,
    };
}

void showCurrentlyPinnedMessage(const std::shared_ptr<TwitchChannel> &chan,
                                const TwitchAccount &moderator)
{
    getHelix()->getPinnedChatMessage(
        chan->roomId(), moderator.getUserId(),
        [weak = std::weak_ptr(chan)](
            const std::optional<HelixPinnedChatMessage> &result) {
            auto chan = weak.lock();
            if (!chan)
            {
                return;
            }

            if (!result)
            {
                chan->addSystemMessage(u"There is no pinned message."_s);
                return;
            }

            chan->addMessage(
                MessageBuilder::makeCurrentPinnedMessage(*chan, *result),
                MessageContext::Original);
        },
        [weak = std::weak_ptr(chan)](const auto &result) {
            auto chan = weak.lock();
            if (!chan)
            {
                return;
            }
            chan->addSystemMessage("Failed to get pinned message: " % result);
        });
}

}  // namespace

namespace chatterino::commands {

/// /pin
QString pin(const CommandContext &ctx)
{
    if (!ctx.channel)
    {
        return {};
    }
    if (!ctx.twitchChannel)
    {
        ctx.channel->addSystemMessage(
            u"The /pin command only works in Twitch channels"_s);
        return {};
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            u"You must be logged in to pin messages!"_s);
        return {};
    }

    if (ctx.words.empty())
    {
        assert(false && "should have /pin");
        return {};
    }

    if (ctx.words.size() == 2 &&
        (ctx.words.at(1) == u"-h" || ctx.words.at(1) == u"--help"))
    {
        ctx.channel->addSystemMessage(
            u"Pin a chat message or show the currently pinned one."
            "Usage: /pin [--duration <seconds|until-end|none>] [message]... OR "
            "/pin --id <message-id> [--duration <seconds|until-end|none>]"_s);
        return {};
    }

    auto chan = std::static_pointer_cast<TwitchChannel>(
        ctx.twitchChannel->shared_from_this());
    if (ctx.words.size() == 1)
    {
        showCurrentlyPinnedMessage(chan, *currentUser);
        return {};
    }

    auto action = parseAction(ctx);
    if (!action)
    {
        ctx.channel->addSystemMessage(action.error());
        return {};
    }

    if (!action->messageID.isEmpty())
    {
        chan->pinMessageAs(action->messageID, action->duration, *currentUser);
        return {};
    }

    sendPinnedMessage(chan, currentUser, action->text, action->duration);
    return {};
}

/// /unpin
QString unpin(const CommandContext &ctx)
{
    if (!ctx.channel)
    {
        return {};
    }
    if (!ctx.twitchChannel)
    {
        ctx.channel->addSystemMessage(
            u"The /unpin command only works in Twitch channels"_s);
        return {};
    }

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        ctx.channel->addSystemMessage(
            u"You must be logged in to unpin messages!"_s);
        return {};
    }

    if (ctx.words.empty())
    {
        assert(false);
        return {};
    }

    auto chan = std::static_pointer_cast<TwitchChannel>(
        ctx.twitchChannel->shared_from_this());
    if (ctx.words.size() == 1)
    {
        getHelix()->getPinnedChatMessage(
            chan->roomId(), currentUser->getUserId(),
            [weak = std::weak_ptr(chan), currentUser](const auto &result) {
                auto chan = weak.lock();
                if (!chan)
                {
                    return;
                }
                if (!result)
                {
                    chan->addSystemMessage(u"There is no pinned message"_s);
                    return;
                }
                chan->unpinMessageAs(result->messageID, *currentUser);
            },
            [weak = std::weak_ptr(chan)](const auto &message) {
                auto chan = weak.lock();
                if (!chan)
                {
                    return;
                }
                chan->addSystemMessage(u"Failed to get pinned message: " %
                                       message);
            });
        return {};
    }

    chan->unpinMessageAs(ctx.words.at(1), *currentUser);
    return {};
}

}  // namespace chatterino::commands
