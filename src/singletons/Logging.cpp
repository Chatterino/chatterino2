#include "singletons/Logging.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <QDir>
#include <QStandardPaths>

#include <unordered_map>

namespace chatterino {

void Logging::initialize()
{
    this->pathManager = getApp()->paths;
}

void Logging::addMessage(const QString &channelName, MessagePtr message)
{
    auto app = getApp();

    if (!app->settings->enableLogging) {
        return;
    }

    auto it = this->loggingChannels.find(channelName);
    if (it == this->loggingChannels.end()) {
        auto channel = new LoggingChannel(channelName);
        channel->addMessage(message);
        this->loggingChannels.emplace(channelName,
                                      std::unique_ptr<LoggingChannel>(std::move(channel)));
    } else {
        it->second->addMessage(message);
    }
}

}  // namespace chatterino
