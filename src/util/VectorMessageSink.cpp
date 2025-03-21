#include "util/VectorMessageSink.hpp"

#include "messages/MessageSimilarity.hpp"
#include "util/ChannelHelpers.hpp"

#include <cassert>

namespace chatterino {

VectorMessageSink::VectorMessageSink(MessageSinkTraits traits,
                                     MessageFlags additionalFlags)
    : additionalFlags(additionalFlags)
    , traits(traits)
{
}

VectorMessageSink::~VectorMessageSink() = default;

void VectorMessageSink::addMessage(MessagePtr message, MessageContext ctx,
                                   std::optional<MessageFlags> overridingFlags)
{
    assert(!overridingFlags.has_value());
    assert(ctx == MessageContext::Original);

    message->flags.set(this->additionalFlags);
    this->messages_.emplace_back(std::move(message));
}

void VectorMessageSink::addOrReplaceTimeout(MessagePtr clearchatMessage,
                                            const QDateTime &now)
{
    addOrReplaceChannelTimeout(
        this->messages_, std::move(clearchatMessage), now,
        [&](auto idx, auto /*msg*/, auto &&replacement) {
            replacement->flags.set(this->additionalFlags);
            this->messages_[idx] = replacement;
        },
        [&](auto &&msg) {
            this->messages_.emplace_back(msg);
        },
        false);
}

void VectorMessageSink::addOrReplaceClearChat(MessagePtr clearchatMessage,
                                              const QDateTime &now)
{
    addOrReplaceChannelClear(
        this->messages_, std::move(clearchatMessage), now,
        [&](auto idx, auto /*msg*/, auto &&replacement) {
            replacement->flags.set(this->additionalFlags);
            this->messages_[idx] = replacement;
        },
        [&](auto &&msg) {
            this->messages_.emplace_back(msg);
        });
}

void VectorMessageSink::disableAllMessages()
{
    if (this->additionalFlags.has(MessageFlag::RecentMessage))
    {
        return;  // don't disable recent messages
    }

    for (const auto &msg : this->messages_)
    {
        msg->flags.set(MessageFlag::Disabled);
    }
}

void VectorMessageSink::applySimilarityFilters(const MessagePtr &message) const
{
    setSimilarityFlags(message, this->messages_);
}

MessagePtr VectorMessageSink::findMessageByID(QStringView id)
{
    for (const auto &msg : this->messages_ | std::views::reverse)
    {
        if (msg->id == id)
        {
            return msg;
        }
    }
    return {};
}

const std::vector<MessagePtr> &VectorMessageSink::messages() const
{
    return this->messages_;
}

std::vector<MessagePtr> VectorMessageSink::takeMessages() &&
{
    return std::move(this->messages_);
}

MessageSinkTraits VectorMessageSink::sinkTraits() const
{
    return this->traits;
}

}  // namespace chatterino
