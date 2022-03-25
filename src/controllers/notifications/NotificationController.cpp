#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Window.hpp"

#ifdef Q_OS_WIN
#    include <wintoastlib.h>
#endif

#include <QDesktopServices>
#include <QDir>
#include <QMediaPlayer>
#include <QUrl>
#include <unordered_set>

namespace chatterino {

void NotificationController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
    for (const QString &channelName : this->twitchSetting_.getValue())
    {
        this->channelMap[Platform::Twitch].append(channelName);
    }

    this->channelMap[Platform::Twitch].delayedItemsChanged.connect([this] {
        this->twitchSetting_.setValue(this->channelMap[Platform::Twitch].raw());
    });
    /*
    for (const QString &channelName : this->mixerSetting_.getValue()) {
        this->channelMap[Platform::Mixer].appendItem(channelName);
    }

    this->channelMap[Platform::Mixer].delayedItemsChanged.connect([this] {
        this->mixerSetting_.setValue(
            this->channelMap[Platform::Mixer]);
    });*/

    liveStatusTimer_ = new QTimer();

    this->fetchFakeChannels();

    QObject::connect(this->liveStatusTimer_, &QTimer::timeout, [=] {
        this->fetchFakeChannels();
    });
    this->liveStatusTimer_->start(60 * 1000);
}

void NotificationController::updateChannelNotification(
    const QString &channelName, Platform p)
{
    if (isChannelNotified(channelName, p))
    {
        removeChannelNotification(channelName, p);
    }
    else
    {
        addChannelNotification(channelName, p);
    }
}

bool NotificationController::isChannelNotified(const QString &channelName,
                                               Platform p)
{
    for (const auto &channel : this->channelMap[p])
    {
        if (channelName.toLower() == channel.toLower())
        {
            return true;
        }
    }
    return false;
}

void NotificationController::addChannelNotification(const QString &channelName,
                                                    Platform p)
{
    channelMap[p].append(channelName);
}

void NotificationController::removeChannelNotification(
    const QString &channelName, Platform p)
{
    for (std::vector<int>::size_type i = 0; i != channelMap[p].raw().size();
         i++)
    {
        if (channelMap[p].raw()[i].toLower() == channelName.toLower())
        {
            channelMap[p].removeAt(i);
            i--;
        }
    }
}
void NotificationController::playSound()
{
    static auto player = new QMediaPlayer;
    static QUrl currentPlayerUrl;

    QUrl highlightSoundUrl =
        getSettings()->notificationCustomSound
            ? QUrl::fromLocalFile(
                  getSettings()->notificationPathSound.getValue())
            : QUrl("qrc:/sounds/ping2.wav");

    if (currentPlayerUrl != highlightSoundUrl)
    {
        player->setMedia(highlightSoundUrl);

        currentPlayerUrl = highlightSoundUrl;
    }
    player->play();
}

NotificationModel *NotificationController::createModel(QObject *parent,
                                                       Platform p)
{
    NotificationModel *model = new NotificationModel(parent);
    model->initialize(&this->channelMap[p]);
    return model;
}

namespace {
    // TODO: combine this with getEmoteSetBatches in TwitchAccount.cpp, maybe some templated thing
    std::vector<QStringList> getChannelsInBatches(QStringList channels)
    {
        constexpr int batchSize = 100;

        int batchCount = (channels.size() / batchSize) + 1;

        std::vector<QStringList> batches;
        batches.reserve(batchCount);

        for (int i = 0; i < batchCount; i++)
        {
            QStringList batch;

            // I hate you, msvc
            int last = (std::min)(batchSize, channels.size() - batchSize * i);
            for (int j = 0; j < last; j++)
            {
                batch.push_back(channels.at(j + (batchSize * i)));
            }
            batches.emplace_back(batch);
        }

        return batches;
    }
}  // namespace

void NotificationController::fetchFakeChannels()
{
    qCDebug(chatterinoNotification) << "fetching fake channels";
    QStringList channels;
    for (std::vector<int>::size_type i = 0;
         i != channelMap[Platform::Twitch].raw().size(); i++)
    {
        auto chan = getApp()->twitch->getChannelOrEmpty(
            channelMap[Platform::Twitch].raw()[i]);
        if (chan->isEmpty())
        {
            channels.push_back(channelMap[Platform::Twitch].raw()[i]);
        }
    }
    for (const auto &batch : getChannelsInBatches(channels))
    {
        getHelix()->fetchStreams(
            QStringList(), batch,
            [batch, this](std::vector<HelixStream> streams) {
                std::unordered_set<QString> liveStreams;
                for (const auto &stream : streams)
                {
                    liveStreams.insert(stream.userName);
                }

                for (const auto &name : batch)
                {
                    auto it = liveStreams.find(name);
                    this->checkStream(it != liveStreams.end(), name);
                }
            },
            [batch]() {
                // we done fucked up.
                qCWarning(chatterinoNotification)
                    << "Failed to fetch live status for " << batch;
            });
    }
}
void NotificationController::checkStream(bool live, QString channelName)
{
    qCDebug(chatterinoNotification)
        << "[TwitchChannel" << channelName << "] Refreshing live status";

    if (!live)
    {
        // Stream is offline
        this->removeFakeChannel(channelName);
        return;
    }

    // Stream is online
    auto i = std::find(fakeTwitchChannels.begin(), fakeTwitchChannels.end(),
                       channelName);

    if (i != fakeTwitchChannels.end())
    {
        // We have already pushed the live state of this stream
        // Could not find stream in fake Twitch channels!
        return;
    }

    if (Toasts::isEnabled())
    {
        getApp()->toasts->sendChannelNotification(channelName,
                                                  Platform::Twitch);
    }
    if (getSettings()->notificationPlaySound &&
        !(isInStreamerMode() &&
          getSettings()->streamerModeSuppressLiveNotifications))
    {
        getApp()->notifications->playSound();
    }
    if (getSettings()->notificationFlashTaskbar &&
        !(isInStreamerMode() &&
          getSettings()->streamerModeSuppressLiveNotifications))
    {
        getApp()->windows->sendAlert();
    }
    MessageBuilder builder;
    TwitchMessageBuilder::liveMessage(channelName, &builder);
    getApp()->twitch->liveChannel->addMessage(builder.release());

    // Indicate that we have pushed notifications for this stream
    fakeTwitchChannels.push_back(channelName);
}

void NotificationController::removeFakeChannel(const QString channelName)
{
    auto i = std::find(fakeTwitchChannels.begin(), fakeTwitchChannels.end(),
                       channelName);
    if (i != fakeTwitchChannels.end())
    {
        fakeTwitchChannels.erase(i);
    }
}

}  // namespace chatterino
