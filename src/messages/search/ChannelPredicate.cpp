#include "messages/search/ChannelPredicate.hpp"

namespace chatterino {

ChannelPredicate::ChannelPredicate(const QStringList &channels)
    : channels_()
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &entry : channels)
    {
        for (const auto &channel : entry.split(',', QString::SkipEmptyParts))
        {
            this->channels_ << channel;
        }
    }
}

bool ChannelPredicate::appliesTo(const Message &message)
{
    return channels_.contains(message.channelName, Qt::CaseInsensitive);
}

}  // namespace chatterino
