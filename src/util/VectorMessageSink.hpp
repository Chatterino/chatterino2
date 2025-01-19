#pragma once

#include "messages/MessageSink.hpp"

namespace chatterino {

class VectorMessageSink final : public MessageSink
{
public:
    VectorMessageSink(MessageSinkTraits traits = {},
                      MessageFlags additionalFlags = {});
    ~VectorMessageSink() override;

    void addMessage(
        MessagePtr message, MessageContext ctx,
        std::optional<MessageFlags> overridingFlags = std::nullopt) override;
    void addOrReplaceTimeout(MessagePtr clearchatMessage,
                             const QDateTime &now) override;
    void addOrReplaceClearChat(MessagePtr clearchatMessage,
                               const QDateTime &now) override;

    void disableAllMessages() override;

    void applySimilarityFilters(const MessagePtr &message) const override;

    MessagePtr findMessageByID(QStringView id) override;

    MessageSinkTraits sinkTraits() const override;

    const std::vector<MessagePtr> &messages() const;
    std::vector<MessagePtr> takeMessages() &&;

private:
    std::vector<MessagePtr> messages_;
    MessageFlags additionalFlags;
    MessageSinkTraits traits;
};

}  // namespace chatterino
