#pragma once

#include "messages/Message.hpp"
#include "singletons/helper/LoggingChannel.hpp"

#include <memory>

namespace chatterino {

class Paths;

class Logging
{
    Paths *pathManager = nullptr;

public:
    Logging() = default;

    ~Logging() = delete;

    void initialize();

    void addMessage(const QString &channelName, MessagePtr message);

private:
    std::map<QString, std::unique_ptr<LoggingChannel>> loggingChannels_;
};

}  // namespace chatterino
