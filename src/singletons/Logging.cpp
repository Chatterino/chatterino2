#include "singletons/Logging.hpp"

#include "singletons/helper/LoggingChannel.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <memory>
#include <utility>

namespace chatterino {

void Logging::initialize(Settings &settings, Paths & /*paths*/)
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
                         const QString &platformName)
{
    this->threadGuard.guard();

    if (getSettings()->onlyLogListedChannels)
    {
        if (!this->onlyLogListedChannels.contains(channelName))
        {
            return;
        }
    }

    this->loggingChannels_.at(platformName)
        .at(channelName)
        ->addMessage(message);
}

void Logging::addChannel(const QString &channelName,
                         const QString &platformName)
{
    if (!this->loggingChannels_.contains(platformName))
    {
        this->loggingChannels_.emplace(
            platformName,
            std::unordered_map<ChannelName, std::unique_ptr<LoggingChannel>>());
    }

    if (!this->loggingChannels_.at(platformName).contains(channelName))
    {
        auto channel = new LoggingChannel(channelName, platformName);
        this->loggingChannels_.at(platformName)
            .emplace(channelName,
                     std::unique_ptr<LoggingChannel>(std::move(channel)));
    }
}

void Logging::removeChannel(const QString &channelName,
                            const QString &platformName)
{
    if (this->loggingChannels_.contains(platformName))
    {
        this->loggingChannels_.at(platformName).erase(channelName);
    }
}

}  // namespace chatterino
