#include "controllers/logging/ChannelLog.hpp"

namespace chatterino {

// ChannelLog
ChannelLog::ChannelLog(const QString &channel, bool loggingEnabled)
    : channel(channel)
    , loggingEnabled(loggingEnabled)
{
}

QString ChannelLog::toString() const
{
    return this->channel;
}

ChannelLog ChannelLog::createEmpty()
{
    return ChannelLog("", false);
}

}  // namespace chatterino
