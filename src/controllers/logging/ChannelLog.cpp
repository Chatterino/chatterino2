#include "controllers/logging/ChannelLog.hpp"

namespace chatterino {

ChannelLog::ChannelLog(QString channelName)
    : channelName_(std::move(channelName))
{
}

QString ChannelLog::channelName() const
{
    return this->channelName_;
}

QString ChannelLog::toString() const
{
    return this->channelName_;
}

ChannelLog ChannelLog::createEmpty()
{
    return {""};
}

}  // namespace chatterino
