#include "singletons/loggingmanager.hpp"

#include "application.hpp"
#include "debug/log.hpp"
#include "singletons/pathmanager.hpp"
#include "singletons/settingsmanager.hpp"

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
