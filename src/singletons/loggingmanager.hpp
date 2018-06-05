#pragma once

#include "messages/message.hpp"
#include "singletons/helper/loggingchannel.hpp"

#include <memory>

namespace chatterino {
namespace singletons {
class PathManager;

class LoggingManager
{
    PathManager *pathManager = nullptr;

public:
    LoggingManager() = default;

    ~LoggingManager() = delete;

    void initialize();

    void addMessage(const QString &channelName, messages::MessagePtr message);

    void refreshLoggingPath();

private:
    std::map<QString, std::unique_ptr<LoggingChannel>> loggingChannels;
    QString getDirectoryForChannel(const QString &channelName);
};

}  // namespace singletons
}  // namespace chatterino
