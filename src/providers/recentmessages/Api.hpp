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
 */
void load(const QString &channelName, std::weak_ptr<Channel> channelPtr,
          ResultCallback onLoaded, ErrorCallback onError);

}  // namespace chatterino::recentmessages
