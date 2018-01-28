#pragma once

#include "messages/message.hpp"
#include "singletons/helper/loggingchannel.hpp"

#include <boost/noncopyable.hpp>

#include <memory>

namespace chatterino {
namespace singletons {

class PathManager;

class LoggingManager : boost::noncopyable
{
    LoggingManager();

    PathManager &pathManager;

public:
    static LoggingManager &getInstance();

    void addMessage(const QString &channelName, messages::MessagePtr message);

private:
    std::map<QString, std::unique_ptr<LoggingChannel>> loggingChannels;
    QString getDirectoryForChannel(const QString &channelName);
};

}  // namespace singletons
}  // namespace chatterino
