#pragma once

#include "common/enums/MessageContext.hpp"
#include "messages/MessageFlag.hpp"

#include <memory>

class QStringView;
class QTime;

namespace chatterino {

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

enum class MessageSinkTrait : uint8_t {
    None = 0,
    AddMentionsToGlobalChannel = 1 << 0,
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
                                     QTime now) = 0;

    /// Flags all messages as `Disabled`
    virtual void disableAllMessages() = 0;

    /// Searches for similar messages and flags this message as similar
    /// (based on the current settings).
    virtual void applySimilarityFilters(const MessagePtr &message) const = 0;

    /// @brief Searches for a message by an ID
    ///
    /// If there is no message found, an empty shared-pointer is returned.
    virtual MessagePtr findMessageByID(QStringView id) = 0;

    ///
    virtual MessageSinkTraits sinkTraits() const = 0;
};

}  // namespace chatterino
