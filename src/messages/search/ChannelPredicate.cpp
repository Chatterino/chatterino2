#include "messages/search/ChannelPredicate.hpp"

#include "messages/Message.hpp"

namespace chatterino {

ChannelPredicate::ChannelPredicate(const QString &channels, bool negate)
    : MessagePredicate(negate)
    , channels_()
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &channel : channels.split(',', Qt::SkipEmptyParts))
    {
        this->channels_ << channel;
    }
}

bool ChannelPredicate::appliesToImpl(const Message &message)
{
    return channels_.contains(message.channelName, Qt::CaseInsensitive);
}

}  // namespace chatterino
