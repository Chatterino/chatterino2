#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "controllers/sound/ISoundController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"

#include <QUrl>

namespace ranges = std::ranges;

namespace chatterino {

NotificationController::NotificationController()
{
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

    QObject::connect(&this->liveStatusTimer_, &QTimer::timeout, [this] {
        this->fetchFakeChannels();
    });
    this->liveStatusTimer_.start(60 * 1000);
}

void NotificationController::initialize()
{
    this->fetchFakeChannels();
}

void NotificationController::updateChannelNotification(
    const QString &channelName, Platform p)
{
    if (this->isChannelNotified(channelName, p))
    {
        this->removeChannelNotification(channelName, p);
    }
    else
    {
        this->addChannelNotification(channelName, p);
    }
}

bool NotificationController::isChannelNotified(const QString &channelName,
                                               Platform p) const
{
    return ranges::any_of(channelMap.at(p).raw(), [&](const auto &name) {
        return name.compare(channelName, Qt::CaseInsensitive) == 0;
    });
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
        if (channelMap[p].raw()[i].compare(channelName, Qt::CaseInsensitive) ==
            0)
        {
            channelMap[p].removeAt(static_cast<int>(i));
            i--;
        }
    }
}

void NotificationController::playSound() const
{
    QUrl highlightSoundUrl =
        getSettings()->notificationCustomSound
            ? QUrl::fromLocalFile(
                  getSettings()->notificationPathSound.getValue())
            : QUrl("qrc:/sounds/ping2.wav");

    getApp()->getSound()->play(highlightSoundUrl);
}

NotificationModel *NotificationController::createModel(QObject *parent,
                                                       Platform p)
{
    auto *model = new NotificationModel(parent);
    model->initialize(&this->channelMap[p]);
    return model;
}

void NotificationController::notifyTwitchChannelLive(
    const NotificationPayload &payload) const
{
    bool showNotification =
        !(getSettings()->suppressInitialLiveNotification &&
          payload.isInitialUpdate) &&
        !(getApp()->getStreamerMode()->isEnabled() &&
          getSettings()->streamerModeSuppressLiveNotifications);
    bool playedSound = false;

    if (showNotification &&
        this->isChannelNotified(payload.channelName, Platform::Twitch))
    {
        if (Toasts::isEnabled())
        {
            getApp()->getToasts()->sendChannelNotification(payload.channelName,
                                                           payload.title);
        }
        if (getSettings()->notificationPlaySound)
        {
            this->playSound();
            playedSound = true;
        }
        if (getSettings()->notificationFlashTaskbar)
        {
            getApp()->getWindows()->sendAlert();
        }
    }

    // Message in /live channel
    getApp()->getTwitch()->getLiveChannel()->addMessage(
        MessageBuilder::makeLiveMessage(payload.displayName, payload.channelId),
        MessageContext::Original);

    // Notify on all channels with a ping sound
    if (showNotification && !playedSound &&
        getSettings()->notificationOnAnyChannel)
    {
        this->playSound();
    }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void NotificationController::notifyTwitchChannelOffline(const QString &id) const
{
    // "delete" old 'CHANNEL is live' message
    LimitedQueueSnapshot<MessagePtr> snapshot =
        getApp()->getTwitch()->getLiveChannel()->getMessageSnapshot();
    int snapshotLength = static_cast<int>(snapshot.size());

    int end = std::max(0, snapshotLength - 200);

    for (int i = snapshotLength - 1; i >= end; --i)
    {
        const auto &s = snapshot[i];

        if (s->id == id)
        {
            s->flags.set(MessageFlag::Disabled);
            break;
        }
    }
}

void NotificationController::fetchFakeChannels()
{
    qCDebug(chatterinoNotification) << "fetching fake channels";

    QStringList channels;
    for (size_t i = 0; i < channelMap[Platform::Twitch].raw().size(); i++)
    {
        const auto &name = channelMap[Platform::Twitch].raw()[i];
        auto chan = getApp()->getTwitch()->getChannelOrEmpty(name);
        if (chan->isEmpty())
        {
            channels.push_back(name);
        }
        else
        {
            this->fakeChannels_.erase(name);
        }
    }

    for (const auto &batch : splitListIntoBatches(channels))
    {
        getHelix()->fetchStreams(
            {}, batch,
            [batch, this](const auto &streams) {
                std::map<QString, std::optional<HelixStream>,
                         QCompareCaseInsensitive>
                    liveStreams;
                for (const auto &stream : streams)
                {
                    liveStreams.emplace(stream.userLogin, stream);
                }

                for (const auto &name : batch)
                {
                    auto it = liveStreams.find(name);
                    if (it == liveStreams.end())
                    {
                        this->updateFakeChannel(name, std::nullopt);
                    }
                    else
                    {
                        this->updateFakeChannel(name, it->second);
                    }
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
void NotificationController::updateFakeChannel(
    const QString &channelName, const std::optional<HelixStream> &stream)
{
    bool live = stream.has_value();
    qCDebug(chatterinoNotification).nospace().noquote()
        << "[FakeTwitchChannel " << channelName
        << "] New live status: " << stream.has_value();

    auto channelIt = this->fakeChannels_.find(channelName);
    bool isInitialUpdate = false;
    if (channelIt == this->fakeChannels_.end())
    {
        channelIt = this->fakeChannels_
                        .emplace(channelName,
                                 FakeChannel{
                                     .id = {},
                                     .isLive = live,
                                 })
                        .first;
        isInitialUpdate = true;
    }
    if (channelIt->second.isLive == live && !isInitialUpdate)
    {
        return;  // nothing changed
    }

    if (live && channelIt->second.id.isNull())
    {
        channelIt->second.id = stream->userId;
    }

    channelIt->second.isLive = live;

    // Similar code can be found in TwitchChannel::onLiveStatusChange.
    // Since this is a fake channel, we don't send a live message in the
    // TwitchChannel.
    if (!live)
    {
        // Stream is offline
        this->notifyTwitchChannelOffline(channelIt->second.id);
        return;
    }

    this->notifyTwitchChannelLive({
        .channelId = stream->userId,
        .channelName = channelName,
        .displayName = stream->userName,
        .title = stream->title,
        .isInitialUpdate = isInitialUpdate,
    });
}

}  // namespace chatterino
