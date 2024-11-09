#pragma once

namespace chatterino {

/// Context of the message being added to a channel
enum class MessageContext {
    /// This message is the original
    Original,
    /// This message is a repost of a message that has already been added in a channel
    Repost,
};

}  // namespace chatterino
