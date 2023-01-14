#include "controllers/logging/ChannelLog.hpp"

namespace chatterino {

// ChannelLog
ChannelLog::ChannelLog(const QString &channel)
    : channel(channel)
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
