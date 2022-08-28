#include "RecentMessagesApi.hpp"

#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Settings.hpp"
#include "util/FormatTime.hpp"
#include "util/PostToThread.hpp"

#include <IrcMessage>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

namespace chatterino {

namespace {

    // convertClearchatToNotice takes a Communi::IrcMessage that is a CLEARCHAT
    // command and converts it to a readable NOTICE message. This has
    // historically been done in the Recent Messages API, but this functionality
    // has been moved to Chatterino instead.
    auto convertClearchatToNotice(Communi::IrcMessage *message)
    {
        auto channelName = message->parameter(0);
        QString noticeMessage{};
        if (message->tags().contains("target-user-id"))
        {
            auto target = message->parameter(1);

            if (message->tags().contains("ban-duration"))
            {
                // User was timed out
                noticeMessage =
                    QString("%1 has been timed out for %2.")
                        .arg(target)
                        .arg(formatTime(
                            message->tag("ban-duration").toString()));
            }
            else
            {
                // User was permanently banned
                noticeMessage =
                    QString("%1 has been permanently banned.").arg(target);
            }
        }
        else
        {
            // Chat was cleared
            noticeMessage = "Chat has been cleared by a moderator.";
        }

        // rebuild the raw IRC message so we can convert it back to an ircmessage again!
        // this could probably be done in a smarter way

        auto s = QString(":tmi.twitch.tv NOTICE %1 :%2")
                     .arg(channelName)
                     .arg(noticeMessage);

        auto newMessage = Communi::IrcMessage::fromData(s.toUtf8(), nullptr);
        newMessage->setTags(message->tags());

        return newMessage;
    }

    // Parse the IRC messages returned in JSON form into Communi messages
    std::vector<Communi::IrcMessage *> parseRecentMessages(
        const QJsonObject &jsonRoot)
    {
        QJsonArray jsonMessages = jsonRoot.value("messages").toArray();
        std::vector<Communi::IrcMessage *> messages;

        if (jsonMessages.empty())
            return messages;

        for (const auto jsonMessage : jsonMessages)
        {
            auto content = jsonMessage.toString();
            content.replace(COMBINED_FIXER, ZERO_WIDTH_JOINER);

            auto message =
                Communi::IrcMessage::fromData(content.toUtf8(), nullptr);

            if (message->command() == "CLEARCHAT")
            {
                message = convertClearchatToNotice(message);
            }

            messages.emplace_back(std::move(message));
        }

        return messages;
    }

    // Build Communi messages retrieved from the recent messages API into
    // proper chatterino messages.
    std::vector<MessagePtr> buildRecentMessages(
        std::vector<Communi::IrcMessage *> &messages, Channel *channel)
    {
        auto &handler = IrcMessageHandler::instance();
        std::vector<MessagePtr> allBuiltMessages;

        for (auto message : messages)
        {
            if (message->tags().contains("rm-received-ts"))
            {
                QDate msgDate =
                    QDateTime::fromMSecsSinceEpoch(
                        message->tags().value("rm-received-ts").toLongLong())
                        .date();

                // Check if we need to insert a message stating that a new day began
                if (msgDate != channel->lastDate_)
                {
                    channel->lastDate_ = msgDate;
                    auto msg = makeSystemMessage(
                        QLocale().toString(msgDate, QLocale::LongFormat),
                        QTime(0, 0));
                    msg->flags.set(MessageFlag::RecentMessage);
                    allBuiltMessages.emplace_back(msg);
                }
            }

            auto builtMessages = handler.parseMessageWithReply(
                channel, message, allBuiltMessages);

            for (auto builtMessage : builtMessages)
            {
                builtMessage->flags.set(MessageFlag::RecentMessage);
                allBuiltMessages.emplace_back(builtMessage);
            }
        }

        return allBuiltMessages;
    }

    // Returns the URL to be used for querying the Recent Messages API for the
    // given channel.
    QUrl constructRecentMessagesUrl(const QString &name)
    {
        QUrl url(Env::get().recentMessagesApiUrl.arg(name));
        QUrlQuery urlQuery(url);
        if (!urlQuery.hasQueryItem("limit"))
        {
            urlQuery.addQueryItem(
                "limit",
                QString::number(getSettings()->twitchMessageHistoryLimit));
        }
        url.setQuery(urlQuery);
        return url;
    }

}  // namespace

void RecentMessagesApi::loadRecentMessages(const QString &channelName,
                                           std::weak_ptr<Channel> channelPtr,
                                           ResultCallback onLoaded,
                                           ErrorCallback onError)
{
    qCDebug(chatterinoRecentMessages)
        << "Loading recent messages for" << channelName;

    QUrl url = constructRecentMessagesUrl(channelName);

    NetworkRequest(url)
        .onSuccess([channelPtr, onLoaded](NetworkResult result) -> Outcome {
            auto shared = channelPtr.lock();
            if (!shared)
                return Failure;

            qCDebug(chatterinoRecentMessages)
                << "Successfully loaded recent messages for"
                << shared->getName();

            auto root = result.parseJson();
            auto parsedMessages = parseRecentMessages(root);

            // build the Communi messages into chatterino messages
            auto builtMessages =
                buildRecentMessages(parsedMessages, shared.get());

            postToThread([shared = std::move(shared), root = std::move(root),
                          messages = std::move(builtMessages),
                          onLoaded]() mutable {
                // Notify user about a possible gap in logs if it returned some messages
                // but isn't currently joined to a channel
                if (QString errorCode = root.value("error_code").toString();
                    !errorCode.isEmpty())
                {
                    qCDebug(chatterinoRecentMessages)
                        << QString("Got error from API: error_code=%1, "
                                   "channel=%2")
                               .arg(errorCode, shared->getName());
                    if (errorCode == "channel_not_joined" && !messages.empty())
                    {
                        shared->addMessage(makeSystemMessage(
                            "Message history service recovering, there may "
                            "be gaps in the message history."));
                    }
                }

                onLoaded(messages);
            });

            return Success;
        })
        .onError([channelPtr, onError](NetworkResult result) {
            auto shared = channelPtr.lock();
            if (!shared)
                return;

            qCDebug(chatterinoRecentMessages)
                << "Failed to load recent messages for" << shared->getName();

            shared->addMessage(makeSystemMessage(
                QString("Message history service unavailable (Error %1)")
                    .arg(result.status())));

            onError();
        })
        .execute();
}

}  // namespace chatterino
