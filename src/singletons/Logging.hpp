#pragma once

#include "util/QStringHash.hpp"
#include "util/ThreadGuard.hpp"

#include <QString>

#include <map>
#include <memory>
#include <unordered_set>

namespace chatterino {

class Settings;
struct Message;
using MessagePtr = std::shared_ptr<const Message>;
class LoggingChannel;

class Logging
{
public:
    Logging(Settings &settings);

    void addMessage(const QString &channelName, MessagePtr message,
                    const QString &platformName);

private:
    using PlatformName = QString;
    using ChannelName = QString;
    std::map<PlatformName,
             std::map<ChannelName, std::unique_ptr<LoggingChannel>>>
        loggingChannels_;

    // Keeps the value of the `loggedChannels` settings
    std::unordered_set<ChannelName> onlyLogListedChannels;
    ThreadGuard threadGuard;
};

}  // namespace chatterino
