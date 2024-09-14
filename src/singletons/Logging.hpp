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

class ILogging
{
public:
    virtual ~ILogging() = default;

    virtual void addMessage(const QString &channelName, MessagePtr message,
                            const QString &platformName,
                            const QString &streamID) = 0;

    virtual void closeChannel(const QString &channelName,
                              const QString &platformName) = 0;
};

class Logging : public ILogging
{
public:
    Logging(Settings &settings);

    void addMessage(const QString &channelName, MessagePtr message,
                    const QString &platformName,
                    const QString &streamID) override;

    void closeChannel(const QString &channelName,
                      const QString &platformName) override;

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
