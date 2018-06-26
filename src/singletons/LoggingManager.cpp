#include "singletons/LoggingManager.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/PathManager.hpp"
#include "singletons/SettingsManager.hpp"

#include <QDir>
#include <QStandardPaths>

#include <unordered_map>

namespace chatterino {
namespace singletons {

void LoggingManager::initialize()
{
    this->pathManager = getApp()->paths;
}

void LoggingManager::addMessage(const QString &channelName, messages::MessagePtr message)
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

}  // namespace singletons
}  // namespace chatterino
