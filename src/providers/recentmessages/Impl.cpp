#include "providers/recentmessages/Impl.hpp"

#include "common/Env.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Settings.hpp"
#include "util/FormatTime.hpp"

#include <QJsonArray>
#include <QUrlQuery>

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoRecentMessages;

}  // namespace

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
        auto content = jsonMessage.toString();

        // For explanation of why this exists, see src/providers/twitch/TwitchChannel.hpp,
        // where these constants are defined
        content.replace(COMBINED_FIXER, ZERO_WIDTH_JOINER);

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
    std::vector<MessagePtr> allBuiltMessages;

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
                msg->flags.set(MessageFlag::RecentMessage);
                allBuiltMessages.emplace_back(msg);
            }
        }

        auto builtMessages = IrcMessageHandler::parseMessageWithReply(
            channel, message, allBuiltMessages);

        for (const auto &builtMessage : builtMessages)
        {
            builtMessage->flags.set(MessageFlag::RecentMessage);
            allBuiltMessages.emplace_back(builtMessage);
        }

        message->deleteLater();
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
            "limit", QString::number(getSettings()->twitchMessageHistoryLimit));
    }
    url.setQuery(urlQuery);
    return url;
}

}  // namespace chatterino::recentmessages::detail
