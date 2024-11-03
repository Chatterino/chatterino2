#include "providers/recentmessages/Impl.hpp"

#include "common/Env.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Helpers.hpp"
#include "util/VectorMessageSink.hpp"

#include <QJsonArray>
#include <QUrlQuery>

namespace chatterino::recentmessages::detail {

// Parse the IRC messages returned in JSON form into Communi messages
std::vector<Communi::IrcMessage *> parseRecentMessages(
    const QJsonObject &jsonRoot)
{
    const auto jsonMessages = jsonRoot.value("messages").toArray();
    std::vector<Communi::IrcMessage *> messages;

    if (jsonMessages.empty())
    {
        return messages;
    }

    for (const auto &jsonMessage : jsonMessages)
    {
        auto content = unescapeZeroWidthJoiner(jsonMessage.toString());

        auto *message =
            Communi::IrcMessage::fromData(content.toUtf8(), nullptr);

        messages.emplace_back(message);
    }

    return messages;
}

// Build Communi messages retrieved from the recent messages API into
// proper chatterino messages.
std::vector<MessagePtr> buildRecentMessages(
    std::vector<Communi::IrcMessage *> &messages, Channel *channel)
{
    VectorMessageSink sink({}, MessageFlag::RecentMessage);

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel);
    if (!twitchChannel)
    {
        return {};
    }

    for (auto *message : messages)
    {
        if (message->tags().contains("rm-received-ts"))
        {
            const auto msgDate =
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
                sink.addMessage(msg, MessageContext::Original);
            }
        }

        IrcMessageHandler::parseMessageInto(message, sink, twitchChannel);

        message->deleteLater();
    }

    return std::move(sink).takeMessages();
}

// Returns the URL to be used for querying the Recent Messages API for the
// given channel.
QUrl constructRecentMessagesUrl(
    const QString &name, const int limit,
    const std::optional<std::chrono::time_point<std::chrono::system_clock>>
        after,
    const std::optional<std::chrono::time_point<std::chrono::system_clock>>
        before)
{
    QUrl url(Env::get().recentMessagesApiUrl.arg(name));
    QUrlQuery urlQuery(url);
    if (!urlQuery.hasQueryItem("limit"))
    {
        urlQuery.addQueryItem("limit", QString::number(limit));
    }
    if (after.has_value())
    {
        urlQuery.addQueryItem(
            "after", QString::number(
                         std::chrono::duration_cast<std::chrono::milliseconds>(
                             after->time_since_epoch())
                             .count()));
    }
    if (before.has_value())
    {
        urlQuery.addQueryItem(
            "before", QString::number(
                          std::chrono::duration_cast<std::chrono::milliseconds>(
                              before->time_since_epoch())
                              .count()));
    }
    url.setQuery(urlQuery);
    return url;
}

}  // namespace chatterino::recentmessages::detail
