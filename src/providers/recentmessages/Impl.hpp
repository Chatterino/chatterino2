#pragma once

#include "common/Channel.hpp"
#include "messages/Message.hpp"

#include <IrcMessage>
#include <QJsonObject>
#include <QString>
#include <QUrl>

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

namespace chatterino::recentmessages::detail {

// Parse the IRC messages returned in JSON form into Communi messages
std::vector<Communi::IrcMessage *> parseRecentMessages(
    const QJsonObject &jsonRoot);

// Build Communi messages retrieved from the recent messages API into
// proper chatterino messages.
std::vector<MessagePtr> buildRecentMessages(
    std::vector<Communi::IrcMessage *> &messages, Channel *channel);

// Returns the URL to be used for querying the Recent Messages API for the
// given channel.
QUrl constructRecentMessagesUrl(
    const QString &name, int limit,
    std::optional<std::chrono::time_point<std::chrono::system_clock>> after,
    std::optional<std::chrono::time_point<std::chrono::system_clock>> before);

}  // namespace chatterino::recentmessages::detail
