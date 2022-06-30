#include "singletons/Logging.hpp"

#include "Application.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/helper/LoggingChannel.hpp"

#include <QDir>
#include <QStandardPaths>

#include <memory>
#include <unordered_map>
#include <utility>

namespace chatterino {

void Logging::initialize(Settings &settings, Paths &paths)
{
}

void Logging::addMessage(const QString &channelName, MessagePtr message,
                         const QString &platformName)
{
    if (!getSettings()->enableLogging)
    {
        return;
    }

    auto platIt = this->loggingChannels_.find(platformName);
    if (platIt == this->loggingChannels_.end())
    {
        auto channel = new LoggingChannel(channelName, platformName);
        channel->addMessage(message);
        auto map = std::map<QString, std::unique_ptr<LoggingChannel>>();
        this->loggingChannels_[platformName] = std::move(map);
        auto &ref = this->loggingChannels_.at(platformName);
        ref.emplace(channelName, std::move(channel));
        return;
    }
    auto chanIt = platIt->second.find(channelName);
    if (chanIt == platIt->second.end())
    {
        auto channel = new LoggingChannel(channelName, platformName);
        channel->addMessage(message);
        platIt->second.emplace(channelName, std::move(channel));
    }
    else
    {
        chanIt->second->addMessage(message);
    }
}

}  // namespace chatterino
