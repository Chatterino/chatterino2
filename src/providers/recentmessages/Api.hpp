#pragma once

#include <QString>

#include <functional>
#include <memory>
#include <vector>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

}  // namespace chatterino

namespace chatterino::recentmessages {

using ResultCallback = std::function<void(const std::vector<MessagePtr> &)>;
using ErrorCallback = std::function<void()>;

/**
 * @brief Loads recent messages for a channel using the Recent Messages API
 *
 * @param channelName Name of Twitch channel
 * @param channelPtr Weak pointer to Channel to use to build messages
 * @param onLoaded Callback taking the built messages as a const std::vector<MessagePtr> &
 * @param onError Callback called when the network request fails
 * @param limit Maximum number of messages to query, `-1` uses twitchMessageHistoryLimit setting
 * @param after Only return messages that were received after this timestamp
 * @param before Only return messages that were received before this timestamp
 */
void load(const QString &channelName, std::weak_ptr<Channel> channelPtr,
          ResultCallback onLoaded, ErrorCallback onError, int limit = -1,
          long after = -1, long before = -1);

}  // namespace chatterino::recentmessages
