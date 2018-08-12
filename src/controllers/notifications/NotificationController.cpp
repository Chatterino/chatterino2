#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationModel.hpp"

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

    for (const QString &channelName : this->mixerSetting_.getValue()) {
        this->channelMap[Platform::Mixer].appendItem(channelName);
    }

    this->channelMap[Platform::Mixer].delayedItemsChanged.connect([this] {  //
        this->mixerSetting_.setValue(
            this->channelMap[Platform::Mixer].getVector());
    });
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
    // for (std::vector<int>::size_type i = 0; i != channelMap[p])
    /*
    if (p == Platform::Twitch) {
        for (std::vector<int>::size_type i = 0;
             i != twitchVector.getVector().size(); i++) {
            if (twitchVector.getVector()[i].toLower() ==
    channelName.toLowercase()) { return true;
            }
        }
    } else if (p == Platform::Mixer) {
        for (std::vector<int>::size_type i = 0;
             i != mixerVector.getVector().size(); i++) {
            if (mixerVector.getVector()[i].toLower() ==
    channelName.toLowercase()) { return true;
            }
        }
    }
    */
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
        if (channelMap[p].getVector()[i].toLower() == channelName) {
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
    if (getApp()->settings->customHighlightSound) {
        highlightSoundUrl = QUrl::fromLocalFile(
            getApp()->settings->pathHighlightSound.getValue());
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
    /*
    if (p == Platform::Twitch) {
        model->init(&this->twitchVector);
    } else if (p == Platform::Mixer) {
        model->init(&this->mixerVector);
    }
    */
    return model;
}

}  // namespace chatterino
