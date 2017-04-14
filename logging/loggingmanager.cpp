#include "loggingmanager.h"

#include <QDir>
#include <QStandardPaths>

#include <unordered_map>

namespace  chatterino {
namespace  logging {

static QString logBasePath;
static QString channelBasePath;
static QString whispersBasePath;
static QString mentionsBasePath;

std::unordered_map<std::string, std::weak_ptr<Channel>> channels;

void init()
{
    // Make sure all folders are properly created
    logBasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                  QDir::separator() + "Logs";
    channelBasePath = logBasePath + QDir::separator() + "Channels";
    whispersBasePath = logBasePath + QDir::separator() + "Whispers";
    mentionsBasePath = logBasePath + QDir::separator() + "Mentions";

    {
        QDir dir(logBasePath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }

    {
        QDir dir(channelBasePath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }

    {
        QDir dir(whispersBasePath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }

    {
        QDir dir(mentionsBasePath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
}

static const QString &getBaseDirectory(const QString &channelName)
{
    if (channelName == "/whispers") {
        return whispersBasePath;
    } else if (channelName == "/mentions") {
        return mentionsBasePath;
    } else {
        return channelBasePath;
    }
}

std::shared_ptr<Channel> get(const QString &channelName)
{
    if (channelName.isEmpty()) {
        return nullptr;
    }

    const QString &baseDirectory = getBaseDirectory(channelName);

    auto channel = channels.find(channelName.toStdString());
    if (channel == std::end(channels)) {
        // This channel is definitely not logged yet.
        // Create shared_ptr that we will return, and store a weak_ptr reference
        std::shared_ptr<Channel> ret =
            std::shared_ptr<Channel>(new Channel(channelName, baseDirectory));

        channels[channelName.toStdString()] = std::weak_ptr<Channel>(ret);

        return ret;
    }

    if (auto ret = channels[channelName.toStdString()].lock()) {
        return ret;
    }

    std::shared_ptr<Channel> ret =
        std::shared_ptr<Channel>(new Channel(channelName, baseDirectory));

    channels[channelName.toStdString()] = std::weak_ptr<Channel>(ret);

    return ret;
}

}  // namespace  logging
}  // namespace  chatterino
