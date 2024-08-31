#include "controllers/logging/ChannelLog.hpp"

namespace chatterino {

ChannelLog::ChannelLog(QString channelName)
    : channelName_(std::move(channelName))
{
}

bool ChannelLog::operator==(const ChannelLog &other) const
{
    return this->channelName_ == other.channelName_;
}

QString ChannelLog::channelName() const
{
    return this->channelName_.toLower();
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
