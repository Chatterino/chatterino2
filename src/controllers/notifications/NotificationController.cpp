#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "providers/twitch/TwitchApi.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/WindowManager.hpp"

#include <wintoastlib.h>

#include <QDesktopServices>
#include <QDir>
#include <QMediaPlayer>
#include <QUrl>

namespace chatterino {

void NotificationController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
    for (const QString &channelName : this->twitchSetting_.getValue()) {
        this->channelMap[Platform::Twitch].appendItem(channelName);
    }

    this->channelMap[Platform::Twitch].delayedItemsChanged.connect([this] {  //
        this->twitchSetting_.setValue(
            this->channelMap[Platform::Twitch].getVector());
    });
    /*
    for (const QString &channelName : this->mixerSetting_.getValue()) {
        this->channelMap[Platform::Mixer].appendItem(channelName);
    }

    this->channelMap[Platform::Mixer].delayedItemsChanged.connect([this] {  //
        this->mixerSetting_.setValue(
            this->channelMap[Platform::Mixer].getVector());
    });
    */

    /*
    connect(liveStatusTimer_, SIGNAL(timeout()), this, SLOT());
    liveStatusTimer_.start(60 * 1000);
    */
    liveStatusTimer_ = new QTimer();
    QObject::connect(liveStatusTimer_, this, SIGNAL(timeout()),
                     SLOT(fetchFakeChannels()));
    connect(liveStatusTimer_, SIGNAL(timeout()), SLOT(fetchFakeChannels()));
    liveStatusTimer_->start(1000);
}

void NotificationController::updateChannelNotification(
    const QString &channelName, Platform p)
{
    if (isChannelNotified(channelName, p)) {
        removeChannelNotification(channelName, p);
    } else {
        addChannelNotification(channelName, p);
    }
}

bool NotificationController::isChannelNotified(const QString &channelName,
                                               Platform p)
{
    for (const auto &channel : this->channelMap[p].getVector()) {
        if (channelName.toLower() == channel.toLower()) {
            return true;
        }
    }
    return false;
}

void NotificationController::addChannelNotification(const QString &channelName,
                                                    Platform p)
{
    channelMap[p].appendItem(channelName);
}

void NotificationController::removeChannelNotification(
    const QString &channelName, Platform p)
{
    for (std::vector<int>::size_type i = 0;
         i != channelMap[p].getVector().size(); i++) {
        if (channelMap[p].getVector()[i].toLower() == channelName.toLower()) {
            channelMap[p].removeItem(i);
            i--;
        }
    }
}
void NotificationController::playSound()
{
    static auto player = new QMediaPlayer;
    static QUrl currentPlayerUrl;

    QUrl highlightSoundUrl;
    if (getApp()->settings->notificationCustomSound) {
        highlightSoundUrl = QUrl::fromLocalFile(
            getApp()->settings->notificationPathSound.getValue());
    } else {
        highlightSoundUrl = QUrl("qrc:/sounds/ping2.wav");
    }
    if (currentPlayerUrl != highlightSoundUrl) {
        player->setMedia(highlightSoundUrl);

        currentPlayerUrl = highlightSoundUrl;
    }
    player->play();
}

NotificationModel *NotificationController::createModel(QObject *parent,
                                                       Platform p)
{
    NotificationModel *model = new NotificationModel(parent);
    model->init(&this->channelMap[p]);
    return model;
}

void NotificationController::fetchFakeChannels()
{
    for (std::vector<int>::size_type i = 0;
         i != channelMap[Platform::Twitch].getVector().size(); i++) {
        auto chan = getApp()->twitch.server->getChannelOrEmpty(
            channelMap[Platform::Twitch].getVector()[i]);

        /*
        auto chan = getApp()->twitch.server->getChannelOrEmpty(chanName);
        if (auto *twitchChannel = dynamic_cast<TwitchChannel *>(chan.get())) {
        if (channelMap[Platform::Twitch].getVector()[i].toLower() ==
        channelName.toLower()) { channelMap[Platform::Twitch].removeItem(i);
            i--;
        }
        }*/
    }
}

void NotificationController::getFakeTwitchChannelLiveStatus(
    const QString &channelName)
{
    TwitchApi::findUserId(channelName, [channelName, this](QString roomID) {
        if (roomID.isEmpty()) {
            Log("[TwitchChannel:{}] Refreshing live status (Missing ID)",
                channelName);
            removeFakeChannel(channelName);
            return;
        }
        Log("[TwitchChannel:{}] Refreshing live status", channelName);

        QString url("https://api.twitch.tv/kraken/streams/" + roomID);
        auto request = NetworkRequest::twitchRequest(url);
        request.setCaller(QThread::currentThread());

        request.onSuccess([this, channelName](auto result) -> Outcome {
            rapidjson::Document document = result.parseRapidJson();
            if (!document.IsObject()) {
                Log("[TwitchChannel:refreshLiveStatus] root is not an object");
                return Failure;
            }

            if (!document.HasMember("stream")) {
                Log("[TwitchChannel:refreshLiveStatus] Missing stream in root");
                return Failure;
            }

            const auto &stream = document["stream"];

            if (!stream.IsObject()) {
                // Stream is offline (stream is most likely null)
                removeFakeChannel(channelName);
                return Failure;
            }
            // Stream is live
            auto i = std::find(fakeTwitchChannels.begin(),
                               fakeTwitchChannels.end(), channelName);
            if (i != fakeTwitchChannels.end()) {
                fakeTwitchChannels.push_back(channelName);
                if (Toasts::isEnabled()) {
                    getApp()->toasts->sendChannelNotification(channelName,
                                                              Platform::Twitch);
                }
                if (getApp()->settings->notificationPlaySound) {
                    getApp()->notifications->playSound();
                }
                if (getApp()->settings->notificationFlashTaskbar) {
                    QApplication::alert(
                        getApp()->windows->getMainWindow().window(), 2500);
                }
            }
        });

        request.execute();
    });
}

void NotificationController::removeFakeChannel(const QString channelName)
{
    auto i = std::find(fakeTwitchChannels.begin(), fakeTwitchChannels.end(),
                       channelName);
    if (i != fakeTwitchChannels.end()) {
        fakeTwitchChannels.erase(i);
    }
}

}  // namespace chatterino
