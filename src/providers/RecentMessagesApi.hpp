#pragma once

#include "ForwardDecl.hpp"

#include <QString>
#include <QUrl>

#include <functional>
#include <memory>
#include <vector>

class QJsonObject;

namespace Communi {

class IrcMessage;

}  // namespace Communi

namespace chatterino::recentmessages::detail {

// convertClearchatToNotice takes a Communi::IrcMessage that is a CLEARCHAT
// command and converts it to a readable NOTICE message. This has
// historically been done in the Recent Messages API, but this functionality
// has been moved to Chatterino instead.
Communi::IrcMessage *convertClearchatToNotice(Communi::IrcMessage *message);

// Parse the IRC messages returned in JSON form into Communi messages
std::vector<Communi::IrcMessage *> parseRecentMessages(
    const QJsonObject &jsonRoot);

// Build Communi messages retrieved from the recent messages API into
// proper chatterino messages.
std::vector<MessagePtr> buildRecentMessages(
    std::vector<Communi::IrcMessage *> &messages, Channel *channel);

// Returns the URL to be used for querying the Recent Messages API for the
// given channel.
QUrl constructRecentMessagesUrl(const QString &name);

}  // namespace chatterino::recentmessages::detail

namespace chatterino {

class RecentMessagesApi
{
public:
    using ResultCallback = std::function<void(const std::vector<MessagePtr> &)>;
    using ErrorCallback = std::function<void()>;

    /**
     * @brief Loads recent messages for a channel using the Recent Messages API
     * 
     * @param channelName Name of Twitch channel
     * @param channelPtr Weak pointer to Channel to use to build messages
     * @param onLoaded Callback taking the built messages as a const std::vector<MessagePtr> &
     * @param onError Callback called when the network request fails
     */
    static void loadRecentMessages(const QString &channelName,
                                   std::weak_ptr<Channel> channelPtr,
                                   ResultCallback onLoaded,
                                   ErrorCallback onError);
};

}  // namespace chatterino
