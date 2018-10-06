#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchApi.hpp"
#include "providers/twitch/TwitchServer.hpp"
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
    });*/

    liveStatusTimer_ = new QTimer();

    this->fetchFakeChannels();

    QObject::connect(this->liveStatusTimer_, &QTimer::timeout,
                     [=] { this->fetchFakeChannels(); });
    this->liveStatusTimer_->start(60 * 1000);
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
    if (getSettings()->notificationCustomSound) {
        highlightSoundUrl = QUrl::fromLocalFile(
            getSettings()->notificationPathSound.getValue());
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
        if (chan->isEmpty()) {
            getFakeTwitchChannelLiveStatus(
                channelMap[Platform::Twitch].getVector()[i]);
        }
    }
}

void NotificationController::getFakeTwitchChannelLiveStatus(
    const QString &channelName)
{
    TwitchApi::findUserId(channelName, [channelName, this](QString roomID) {
        if (roomID.isEmpty()) {
            log("[TwitchChannel:{}] Refreshing live status (Missing ID)",
                channelName);
            removeFakeChannel(channelName);
            return;
        }
        log("[TwitchChannel:{}] Refreshing live status", channelName);

        QString url("https://api.twitch.tv/kraken/streams/" + roomID);
        auto request = NetworkRequest::twitchRequest(url);
        request.setCaller(QThread::currentThread());

        request.onSuccess([this, channelName](auto result) -> Outcome {
            rapidjson::Document document = result.parseRapidJson();
            if (!document.IsObject()) {
                log("[TwitchChannel:refreshLiveStatus]root is not an object");
                return Failure;
            }

            if (!document.HasMember("stream")) {
                log("[TwitchChannel:refreshLiveStatus] Missing stream in root");
                return Failure;
            }

            const auto &stream = document["stream"];

            if (!stream.IsObject()) {
                // Stream is offline (stream is most likely null)
                // removeFakeChannel(channelName);
                return Failure;
            }
            // Stream is live
            auto i = std::find(fakeTwitchChannels.begin(),
                               fakeTwitchChannels.end(), channelName);

            if (!(i != fakeTwitchChannels.end())) {
                fakeTwitchChannels.push_back(channelName);
                if (Toasts::isEnabled()) {
                    getApp()->toasts->sendChannelNotification(channelName,
                                                              Platform::Twitch);
                }
                if (getSettings()->notificationPlaySound) {
                    getApp()->notifications->playSound();
                }
                if (getSettings()->notificationFlashTaskbar) {
                    int flashDuration = 2500;
                    if (getSettings()->longAlerts) {
                        flashDuration = 0;
                    }
                    QApplication::alert(
                        getApp()->windows->getMainWindow().window(),
                        flashDuration);
                }
            }
            return Success;
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
