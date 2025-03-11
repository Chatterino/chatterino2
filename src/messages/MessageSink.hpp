#pragma once

#include "common/enums/MessageContext.hpp"
#include "common/FlagsEnum.hpp"
#include "messages/MessageFlag.hpp"

#include <memory>
#include <optional>

class QStringView;
class QDateTime;

namespace chatterino {

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

enum class MessageSinkTrait : uint8_t {
    None = 0,

    /// Messages with the `Highlighted` and `ShowInMentions` flags should be
    /// added to the global mentions channel when encountered.
    AddMentionsToGlobalChannel = 1 << 0,

    /// A channel-point redemption whose reward is not yet known should be
    /// queued in the corresponding TwitchChannel (`addQueuedRedemption`) and
    /// the message should be replaced later.
    RequiresKnownChannelPointReward = 1 << 1,
};
using MessageSinkTraits = FlagsEnum<MessageSinkTrait>;

/// A generic interface for a managed buffer of `Message`s
class MessageSink
{
public:
    virtual ~MessageSink() = default;

    /// Add a message to this sink
    ///
    /// @param message The message to add (non-null)
    /// @param ctx The context in which this message is being added.
    /// @param overridingFlags
    virtual void addMessage(
        MessagePtr message, MessageContext ctx,
        std::optional<MessageFlags> overridingFlags = std::nullopt) = 0;

    /// Adds a timeout message or merges it into an existing one
    virtual void addOrReplaceTimeout(MessagePtr clearchatMessage,
                                     const QDateTime &now) = 0;

    /// Adds a clear chat message (for the entire chat) or merges it into an
    /// existing one
    virtual void addOrReplaceClearChat(MessagePtr clearchatMessage,
                                       const QDateTime &now) = 0;

    /// Flags all messages as `Disabled`
    virtual void disableAllMessages() = 0;

    /// Searches for similar messages and flags this message as similar
    /// (based on the current settings).
    virtual void applySimilarityFilters(const MessagePtr &message) const = 0;

    /// @brief Searches for a message by an ID
    ///
    /// If there is no message found, an empty shared-pointer is returned.
    virtual MessagePtr findMessageByID(QStringView id) = 0;

    /// Behaviour to be exercised when parsing/building messages for this sink.
    virtual MessageSinkTraits sinkTraits() const = 0;
};

}  // namespace chatterino
