#include "Toasts.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "util/StreamLink.hpp"
#include "widgets/dialogs/NotificationPopup.hpp"
#include "widgets/helper/CommonTexts.hpp"

#include <QDesktopServices>
#include <QFileInfo>
#include <QLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

#include <cstdlib>

namespace chatterino {

std::map<ToastReaction, QString> Toasts::reactionToString = {
    {ToastReaction::OpenInBrowser, OPEN_IN_BROWSER},
    {ToastReaction::OpenInPlayer, OPEN_PLAYER_IN_BROWSER},
    {ToastReaction::OpenInStreamlink, OPEN_IN_STREAMLINK},
    {ToastReaction::DontOpen, DONT_OPEN}};

QString Toasts::findStringFromReaction(const ToastReaction &reaction)
{
    auto iterator = Toasts::reactionToString.find(reaction);
    if (iterator != Toasts::reactionToString.end())
    {
        return iterator->second;
    }
    else
    {
        return DONT_OPEN;
    }
}

QString Toasts::findStringFromReaction(
    const pajlada::Settings::Setting<int> &value)
{
    int i = static_cast<int>(value);
    return Toasts::findStringFromReaction(static_cast<ToastReaction>(i));
}

void Toasts::sendToastMessage(const QString &channelName)
{
    QString url("https://api.twitch.tv/helix/users?login=" + channelName);

    NetworkRequest::twitchRequest(url)
        .onSuccess([this, channelName](auto result) -> Outcome {
            auto obj = result.parseJson();
            this->actuallySendToastMessage(QUrl(obj.value("data")
                                                    .toArray()[0]
                                                    .toObject()
                                                    .value("profile_image_url")
                                                    .toString()),
                                           channelName);

            return Success;
        })
        .execute();
}

void Toasts::actuallySendToastMessage(const QUrl &url,
                                      const QString &channelName)
{
    QString bottomText = "";
    if (static_cast<ToastReaction>(getSettings()->openFromToast.getValue()) !=
        ToastReaction::DontOpen)
    {
        QString mode =
            Toasts::findStringFromReaction(getSettings()->openFromToast)
                .toLower();

        bottomText = "Click here to " + mode;
    }

    auto callback = [channelName]() {
        switch (
            static_cast<ToastReaction>(getSettings()->openFromToast.getValue()))
        {
            case ToastReaction::OpenInBrowser:
                QDesktopServices::openUrl(
                    QUrl("https://twitch.tv/" + channelName));
                break;
            case ToastReaction::OpenInPlayer:
                QDesktopServices::openUrl(
                    QUrl("https://player.twitch.tv/?channel=" + channelName));
                break;
            case ToastReaction::OpenInStreamlink:
                openStreamlinkForChannel(channelName);
                break;
        }
    };

    NetworkRequest::twitchRequest(url)
        .onSuccess([channelName, bottomText, callback,
                    this](auto result) -> Outcome {
            const auto data = result.getData();

            QPixmap avatar;
            avatar.loadFromData(data);

            getApp()->notifications->addNotification(
                makeLayout(
                    avatar,
                    QString("<b>" + channelName + "</b> just went live!"),
                    bottomText),
                std::chrono::milliseconds(getSettings()->notificationDuration *
                                          1000),
                callback);

            return Success;
        })
        .onError([channelName, bottomText, callback,
                  this](auto result) -> bool {
            getApp()->notifications->addNotification(
                makeLayout(
                    getResources().icon,
                    QString("<b>" + channelName + "</b> just went live!"),
                    bottomText),
                std::chrono::milliseconds(getSettings()->notificationDuration *
                                          1000),
                callback);
            return false;
        })
        .execute();
}

QHBoxLayout *Toasts::makeLayout(const QPixmap &image, const QString &text,
                                const QString &bottomText)
{
    auto *layout = new QHBoxLayout();

    auto *imageLabel = new QLabel();
    imageLabel->setPixmap(image);
    imageLabel->setScaledContents(true);
    imageLabel->setMinimumSize(1, 1);
    imageLabel->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::Minimum);
    layout->addWidget(imageLabel, 1);

    auto *vbox = new QVBoxLayout();
    layout->addLayout(vbox, 2);

    auto *textLabel = new QLabel();
    textLabel->setText(text);
    vbox->addWidget(textLabel);

    auto *bottomTextLabel = new QLabel();
    bottomTextLabel->setText(bottomText);
    vbox->addWidget(bottomTextLabel);

    return layout;
}
}  // namespace chatterino
