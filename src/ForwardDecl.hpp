#pragma once

// This file contains common forward declarations.

namespace chatterino {
class Channel;
class ChannelView;
using ChannelPtr = std::shared_ptr<Channel>;

struct Message;
using MessagePtr = std::shared_ptr<const Message>;
}  // namespace chatterino
