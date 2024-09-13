#include "singletons/Logging.hpp"

#include "messages/Message.hpp"
#include "singletons/helper/LoggingChannel.hpp"
#include "singletons/Settings.hpp"

#include <QDir>
#include <QStandardPaths>

#include <memory>
#include <utility>

namespace chatterino {

Logging::Logging(Settings &settings)
{
    // We can safely ignore this signal connection since settings are only-ever destroyed
    // on application exit
    // NOTE: SETTINGS_LIFETIME
    std::ignore = settings.loggedChannels.delayedItemsChanged.connect(
        [this, &settings]() {
            this->threadGuard.guard();

            this->onlyLogListedChannels.clear();

            for (const auto &loggedChannel :
                 *settings.loggedChannels.readOnly())
            {
                this->onlyLogListedChannels.insert(loggedChannel.channelName());
            }
        });
}

void Logging::addMessage(const QString &channelName, MessagePtr message,
                         const QString &platformName, const QString &streamID)
{
    this->threadGuard.guard();

    if (!getSettings()->enableLogging)
    {
        return;
    }

    if (getSettings()->onlyLogListedChannels)
    {
        if (!this->onlyLogListedChannels.contains(channelName))
        {
            return;
        }
    }

    auto platIt = this->loggingChannels_.find(platformName);
    if (platIt == this->loggingChannels_.end())
    {
        auto *channel = new LoggingChannel(channelName, platformName);
        channel->addMessage(message, streamID);
        auto map = std::map<QString, std::unique_ptr<LoggingChannel>>();
        this->loggingChannels_[platformName] = std::move(map);
        auto &ref = this->loggingChannels_.at(platformName);
        ref.emplace(channelName, channel);
        return;
    }
    auto chanIt = platIt->second.find(channelName);
    if (chanIt == platIt->second.end())
    {
        auto *channel = new LoggingChannel(channelName, platformName);
        channel->addMessage(message, streamID);
        platIt->second.emplace(channelName, channel);
    }
    else
    {
        chanIt->second->addMessage(message, streamID);
    }
}

void Logging::closeChannel(const QString &channelName,
                           const QString &platformName)
{
    auto platIt = this->loggingChannels_.find(platformName);
    if (platIt == this->loggingChannels_.end())
    {
        return;
    }
    platIt->second.erase(channelName);
}

}  // namespace chatterino
