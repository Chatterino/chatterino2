#pragma once

#include "common/Singleton.hpp"

#include "messages/Message.hpp"
#include "singletons/helper/LoggingChannel.hpp"

#include <memory>

namespace chatterino {

class Paths;

class Logging : public Singleton
{
    Paths *pathManager = nullptr;

public:
    Logging() = default;

    virtual void initialize(Settings &settings, Paths &paths) override;

    void addMessage(const QString &channelName, MessagePtr message,
                    const QString &platformName);

private:
    std::map<QString, std::unique_ptr<LoggingChannel>> loggingChannels_;
};

}  // namespace chatterino
