#pragma once

#include "common/Channel.hpp"
#include "messages/Message.hpp"

#include <IrcMessage>
#include <QJsonObject>
#include <QString>
#include <QUrl>

#include <memory>
#include <vector>

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
