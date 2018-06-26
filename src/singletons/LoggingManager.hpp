#pragma once

#include "messages/Message.hpp"
#include "singletons/helper/LoggingChannel.hpp"

#include <memory>

namespace chatterino {

class PathManager;

class LoggingManager
{
    PathManager *pathManager = nullptr;

public:
    LoggingManager() = default;

    ~LoggingManager() = delete;

    void initialize();

    void addMessage(const QString &channelName, chatterino::MessagePtr message);

private:
    std::map<QString, std::unique_ptr<LoggingChannel>> loggingChannels;
};

}  // namespace chatterino
