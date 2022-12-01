#include "messages/search/ChannelPredicate.hpp"

#include "util/Qt.hpp"

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

bool ChannelPredicate::appliesTo(const Message &message)
{
    return this->isNegated ^
           channels_.contains(message.channelName, Qt::CaseInsensitive);
}

}  // namespace chatterino
