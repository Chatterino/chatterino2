#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "providers/twitch/TwitchApi.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/NotificationPopup.hpp"

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QMediaPlayer>
#include <QUrl>

namespace chatterino {

void NotificationController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
    for (const QString &channelName : this->twitchSetting_.getValue())
    {
        this->channelMap[Platform::Twitch].append(channelName);
    }

    this->channelMap[Platform::Twitch].delayedItemsChanged.connect([this] {  //
        this->twitchSetting_.setValue(this->channelMap[Platform::Twitch].raw());
    });
    /*
    for (const QString &channelName : this->mixerSetting_.getValue()) {
        this->channelMap[Platform::Mixer].appendItem(channelName);
    }

    this->channelMap[Platform::Mixer].delayedItemsChanged.connect([this] {  //
        this->mixerSetting_.setValue(
            this->channelMap[Platform::Mixer]);
    });*/

    liveStatusTimer_ = new QTimer();

    this->fetchFakeChannels();

    QObject::connect(this->liveStatusTimer_, &QTimer::timeout,
                     [=] { this->fetchFakeChannels(); });
    this->liveStatusTimer_->start(60 * 1000);

    popupWindow_ = new NotificationPopup();
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

void NotificationController::fetchFakeChannels()
{
    for (std::vector<int>::size_type i = 0;
         i != channelMap[Platform::Twitch].raw().size(); i++)
    {
        auto chan = getApp()->twitch.server->getChannelOrEmpty(
            channelMap[Platform::Twitch].raw()[i]);
        if (chan->isEmpty())
        {
            getFakeTwitchChannelLiveStatus(
                channelMap[Platform::Twitch].raw()[i]);
        }
    }
}

void NotificationController::getFakeTwitchChannelLiveStatus(
    const QString &channelName)
{
    TwitchApi::findUserId(channelName, [channelName, this](QString roomID) {
        if (roomID.isEmpty())
        {
            qDebug() << "[TwitchChannel" << channelName
                     << "] Refreshing live status (Missing ID)";
            removeFakeChannel(channelName);
            return;
        }
        qDebug() << "[TwitchChannel" << channelName
                 << "] Refreshing live status";

        QString url("https://api.twitch.tv/kraken/streams/" + roomID);
        NetworkRequest::twitchRequest(url)
            .onSuccess([this, channelName](auto result) -> Outcome {
                rapidjson::Document document = result.parseRapidJson();
                if (!document.IsObject())
                {
                    qDebug() << "[TwitchChannel:refreshLiveStatus] root is not "
                                "an object";
                    return Failure;
                }

                if (!document.HasMember("stream"))
                {
                    qDebug() << "[TwitchChannel:refreshLiveStatus] Missing "
                                "stream in root";
                    return Failure;
                }

                const auto &stream = document["stream"];

                if (!stream.IsObject())
                {
                    // Stream is offline (stream is most likely null)
                    // removeFakeChannel(channelName);
                    return Failure;
                }
                // Stream is live
                auto i = std::find(fakeTwitchChannels.begin(),
                                   fakeTwitchChannels.end(), channelName);

                if (!(i != fakeTwitchChannels.end()))
                {
                    fakeTwitchChannels.push_back(channelName);
                    if (getSettings()->notificationToast)
                    {
                        getApp()->toasts->sendToastMessage(channelName);
                    }
                    if (getSettings()->notificationPlaySound)
                    {
                        getApp()->notifications->playSound();
                    }
                    if (getSettings()->notificationFlashTaskbar)
                    {
                        getApp()->windows->sendAlert();
                    }
                }
                return Success;
            })
            .execute();
    });
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

void NotificationController::addNotification(QLayout *layout,
                                             std::chrono::milliseconds time,
                                             std::function<void()> callback)
{
    queue_.push({layout, time, callback});
    if (queue_.size() == 1)
    {
        startNotification();
    }
}

void NotificationController::startNotification()
{
    auto finishNotification = [this]() {
        queue_.pop();

        popupWindow_->hide();
        popupWindow_->mouseRelease.disconnectAll();

        if (queue_.size() == 0)
        {
            return;
        }

        startNotification();
    };
    popupWindow_->updatePosition();

    if (auto layout = popupWindow_->layout(); layout)
    {
        qDeleteAll(popupWindow_->findChildren<QWidget *>(
            QString(), Qt ::FindDirectChildrenOnly));
        delete layout;
    }
    auto callback = std::get<2>(queue_.front());

    auto timer = new QTimer();
    timer->callOnTimeout(finishNotification);
    timer->setSingleShot(true);
    timer->start(std::get<1>(queue_.front()));

    popupWindow_->setLayout(std::get<0>(queue_.front()));
    popupWindow_->mouseRelease.connect(
        [finishNotification, timer, callback, this](QMouseEvent *event) {
            if (event->button() == Qt::LeftButton)
            {
                timer->stop();
                finishNotification();
                callback();
            }
        });
    popupWindow_->show();
}

}  // namespace chatterino
