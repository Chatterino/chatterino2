#include "controllers/logging/ChannelLog.hpp"

namespace chatterino {

ChannelLog::ChannelLog(QString channel_)
    : channel(std::move(channel_))
{
}

QString ChannelLog::toString() const
{
    return this->channel;
}

ChannelLog ChannelLog::createEmpty()
{
    return ChannelLog("");
}

}  // namespace chatterino
