#include "singletons/Logging.hpp"

#include "Application.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <QDir>
#include <QStandardPaths>

#include <unordered_map>

namespace chatterino {

void Logging::initialize(Settings &settings, Paths &paths)
{
}

void Logging::addMessage(const QString &channelName, MessagePtr message)
{
    if (!getSettings()->enableLogging)
    {
        return;
    }

    auto it = this->loggingChannels_.find(channelName);
    if (it == this->loggingChannels_.end())
    {
        auto channel = new LoggingChannel(channelName);
        channel->addMessage(message);
        this->loggingChannels_.emplace(
            channelName, std::unique_ptr<LoggingChannel>(std::move(channel)));
    }
    else
    {
        it->second->addMessage(message);
    }
}

}  // namespace chatterino
