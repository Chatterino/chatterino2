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
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"

#include <controllers/commands/builtin/twitch/Pin.hpp>
#include <QCommandLineParser>
#include <QProcess>

namespace {

using namespace Qt::Literals;
using namespace chatterino;

struct SendPinnedAction {
    QString text;

    void run(const std::shared_ptr<TwitchChannel> &chan,
             const TwitchAccount &moderator) const;
};

struct PinAction {
    QString messageID;
    std::optional<std::chrono::seconds> duration;

    void run(const std::shared_ptr<TwitchChannel> &chan,
             const TwitchAccount &moderator) const;
};

using Action = std::variant<SendPinnedAction, PinAction>;

void SendPinnedAction::run(const std::shared_ptr<TwitchChannel> &chan,
                           const TwitchAccount &moderator) const
{
    getHelix()->sendChatMessage(
        {
            .broadcasterID = chan->roomId(),
            .senderID = moderator.getUserId(),
            .message = this->text,
            .pin = true,
        },
        [weak = std::weak_ptr(chan),
         origText = this->text](const HelixSentMessage &res) {
            auto chan = weak.lock();
            if (!chan)
            {
                return;
            }

            if (res.isSent)
            {
                chan->addMessage(
                    MessageBuilder::makePinSuccessMessage(origText, res.id),
                    MessageContext::Original);
                return;
            }

            if (res.dropReason)
            {
                chan->addSystemMessage(res.dropReason->message);
            }
            else
            {
                chan->addSystemMessage("Your message was not pinned.");
            }
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

void PinAction::run(const std::shared_ptr<TwitchChannel> &chan,
                    const TwitchAccount &moderator) const
{
    chan->pinMessageAs(this->messageID, this->duration, moderator);
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
    if (parser.isSet(durationOption))
    {
        const auto dur = parseDurationToSeconds(parser.value(durationOption));
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

    auto positionals = parser.positionalArguments();

    if (id)
    {
        if (!positionals.empty())
        {
            return makeUnexpected(
                u"No positional arguments can be specified when using --id."_s);
        }

        return PinAction{
            .messageID = *std::move(id),
            .duration = duration,
        };
    }

    if (duration)
    {
        return makeUnexpected(
            u"No duration can be specified when sending and pinning a message. "
            "Send the message first, and pin it afterward."_s);
    }

    return SendPinnedAction{
        .text = positionals.join(' '),
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

            MessageBuilder builder;
            builder->channelName = chan->getName();
            builder.emplace<TimestampElement>();
            builder->flags.set(MessageFlag::System,
                               MessageFlag::DoNotTriggerNotification);

            QString text = result->pinnedBy.login + ' ';
            builder.emplace<MentionElement>(
                result->pinnedBy.displayName, result->pinnedBy.login,
                MessageColor::System,
                chan->getUserColor(result->pinnedBy.login));
            builder.emplaceSystemTextAndUpdate("pinned a message", text);

            auto now = QDateTime::currentDateTimeUtc();
            builder.appendOrEmplaceSystemTextAndUpdate(
                formatTime(std::chrono::duration_cast<std::chrono::seconds>(
                    now - result->startsAt)),
                text);
            builder.appendOrEmplaceSystemTextAndUpdate("ago", text);
            if (result->endsAt)
            {
                auto remaining =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        *result->endsAt - now);
                builder.appendOrEmplaceSystemTextAndUpdate(
                    '(' % formatTime(remaining) % " remaining)", text);
            }
            else
            {
                builder.appendOrEmplaceSystemTextAndUpdate(
                    "until the stream ends", text);
            }
            builder.appendOrEmplaceSystemTextAndUpdate("from", text);
            builder
                .emplace<MentionElement>(
                    result->sender.displayName, result->sender.login,
                    MessageColor::System,
                    chan->getUserColor(result->sender.login))
                ->setTrailingSpace(false);
            text += result->sender.login;
            builder.appendOrEmplaceSystemTextAndUpdate(u":"_s, text);

            auto pinMessageText = result->messageText;
            if (pinMessageText.length() > 50)
            {
                pinMessageText = pinMessageText.left(50) + "…";
            }

            builder
                .emplace<TextElement>(pinMessageText, MessageElementFlag::Text,
                                      MessageColor::Text)
                ->setLink({Link::JumpToMessage, result->messageID});
            builder->messageText = pinMessageText;
            builder->searchText = pinMessageText;
            chan->addMessage(builder.release(), MessageContext::Original);
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
        ctx.channel->addSystemMessage("You must be logged in to pin messages!");
        return "";
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
            u"Pin a chat message or show the currently pinned one. Usage: /pin --duration <seconds> --id <id> [message]..."_s);
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

    std::visit(
        [&](const auto &action) {
            action.run(chan, *currentUser);
        },
        *action);

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
        ctx.channel->addSystemMessage("You must be logged in to pin messages!");
        return "";
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
