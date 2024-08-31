#pragma once

#include <QString>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
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
 * @param limit Maximum number of messages to query
 * @param after Only return messages that were received after this timestamp; ignored if `std::nullopt`
 * @param before Only return messages that were received before this timestamp; ignored if `std::nullopt`
 * @param jitter Whether to delay the request by a small random duration
 */
void load(
    const QString &channelName, std::weak_ptr<Channel> channelPtr,
    ResultCallback onLoaded, ErrorCallback onError, int limit,
    std::optional<std::chrono::time_point<std::chrono::system_clock>> after,
    std::optional<std::chrono::time_point<std::chrono::system_clock>> before,
    bool jitter);

}  // namespace chatterino::recentmessages
