#include "controllers/logging/ChannelLog.hpp"

namespace chatterino {

ChannelLog::ChannelLog(QString channelName)
    : channelName_(std::move(channelName))
{
}

QString ChannelLog::channelName() const
{
    QString lowercaseChannelName = this->channelName_.toLower();
    return lowercaseChannelName;
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
