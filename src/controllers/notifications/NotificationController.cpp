#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "controllers/sound/ISoundController.hpp"
#include "messages/Message.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "widgets/Window.hpp"

#ifdef Q_OS_WIN
#    include <wintoastlib.h>
#endif

#include <QDesktopServices>
#include <QDir>
#include <QUrl>

#include <unordered_set>

namespace chatterino {

void NotificationController::initialize(Settings &settings, const Paths &paths)
{
    this->initialized_ = true;
    for (const QString &channelName : this->twitchSetting_.getValue())
    {
        this->channelMap[Platform::Twitch].append(channelName);
    }

    // We can safely ignore this signal connection since channelMap will always be destroyed
    // before the NotificationController
    std::ignore =
        this->channelMap[Platform::Twitch].delayedItemsChanged.connect([this] {
            this->twitchSetting_.setValue(
                this->channelMap[Platform::Twitch].raw());
        });

    liveStatusTimer_ = new QTimer();

    this->fetchFakeChannels();

    QObject::connect(this->liveStatusTimer_, &QTimer::timeout, [this] {
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
    QUrl highlightSoundUrl =
        getSettings()->notificationCustomSound
            ? QUrl::fromLocalFile(
                  getSettings()->notificationPathSound.getValue())
            : QUrl("qrc:/sounds/ping2.wav");

    getIApp()->getSound()->play(highlightSoundUrl);
}

NotificationModel *NotificationController::createModel(QObject *parent,
                                                       Platform p)
{
    NotificationModel *model = new NotificationModel(parent);
    model->initialize(&this->channelMap[p]);
    return model;
}

void NotificationController::fetchFakeChannels()
{
    qCDebug(chatterinoNotification) << "fetching fake channels";
    QStringList channels;
    for (std::vector<int>::size_type i = 0;
         i < channelMap[Platform::Twitch].raw().size(); i++)
    {
        auto chan = getApp()->twitch->getChannelOrEmpty(
            channelMap[Platform::Twitch].raw()[i]);
        if (chan->isEmpty())
        {
            channels.push_back(channelMap[Platform::Twitch].raw()[i]);
        }
    }

    for (const auto &batch : splitListIntoBatches(channels))
    {
        getHelix()->fetchStreams(
            {}, batch,
            [batch, this](std::vector<HelixStream> streams) {
                std::unordered_set<QString> liveStreams;
                for (const auto &stream : streams)
                {
                    liveStreams.insert(stream.userLogin);
                }

                for (const auto &name : batch)
                {
                    auto it = liveStreams.find(name.toLower());
                    this->checkStream(it != liveStreams.end(), name);
                }
            },
            [batch]() {
                // we done fucked up.
                qCWarning(chatterinoNotification)
                    << "Failed to fetch live status for " << batch;
            },
            []() {
                // finally
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
        getIApp()->getToasts()->sendChannelNotification(channelName, QString(),
                                                        Platform::Twitch);
    }
    bool inStreamerMode = getIApp()->getStreamerMode()->isEnabled();
    if (getSettings()->notificationPlaySound &&
        !(inStreamerMode &&
          getSettings()->streamerModeSuppressLiveNotifications))
    {
        getIApp()->getNotifications()->playSound();
    }
    if (getSettings()->notificationFlashTaskbar &&
        !(inStreamerMode &&
          getSettings()->streamerModeSuppressLiveNotifications))
    {
        getIApp()->getWindows()->sendAlert();
    }
    MessageBuilder builder;
    TwitchMessageBuilder::liveMessage(channelName, &builder);
    getIApp()->getTwitch()->getLiveChannel()->addMessage(builder.release());

    // Indicate that we have pushed notifications for this stream
    fakeTwitchChannels.push_back(channelName);
}

void NotificationController::removeFakeChannel(const QString channelName)
{
    auto it = std::find(fakeTwitchChannels.begin(), fakeTwitchChannels.end(),
                        channelName);
    if (it != fakeTwitchChannels.end())
    {
        fakeTwitchChannels.erase(it);
        // "delete" old 'CHANNEL is live' message
        LimitedQueueSnapshot<MessagePtr> snapshot =
            getIApp()->getTwitch()->getLiveChannel()->getMessageSnapshot();
        int snapshotLength = snapshot.size();

        // MSVC hates this code if the parens are not there
        int end = (std::max)(0, snapshotLength - 200);
        // this assumes that channelName is a login name therefore will only delete messages from fake channels
        auto liveMessageSearchText = QString("%1 is live!").arg(channelName);

        for (int i = snapshotLength - 1; i >= end; --i)
        {
            const auto &s = snapshot[i];

            if (QString::compare(s->messageText, liveMessageSearchText,
                                 Qt::CaseInsensitive) == 0)
            {
                s->flags.set(MessageFlag::Disabled);
                break;
            }
        }
    }
}

}  // namespace chatterino
