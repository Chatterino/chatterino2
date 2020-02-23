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

Toasts::Toasts()
{
}

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

    auto onMousePressed = [channelName](NotificationPopup *popup) {
        return [channelName, popup](QMouseEvent *event) {
            // dont know if this is safe
            popup->hide();
            if (event->button() == Qt::LeftButton)
                switch (static_cast<ToastReaction>(
                    getSettings()->openFromToast.getValue()))
                {
                    case ToastReaction::OpenInBrowser:
                        QDesktopServices::openUrl(
                            QUrl("https://twitch.tv/" + channelName));
                        break;
                    case ToastReaction::OpenInPlayer:
                        QDesktopServices::openUrl(
                            QUrl("https://player.twitch.tv/?channel=" +
                                 channelName));
                        break;
                    case ToastReaction::OpenInStreamlink:
                        openStreamlinkForChannel(channelName);
                        break;
                }
        };
    };

    NetworkRequest::twitchRequest(url)
        .onSuccess([channelName, bottomText,
                    onMousePressed](auto result) -> Outcome {
            const auto data = result.getData();

            QPixmap avatar;
            avatar.loadFromData(data);

            auto *popup = new NotificationPopup();
            popup->updatePosition();
            popup->setImageAndText(
                avatar, QString(channelName + " just went live!"), bottomText);
            popup->show();
            popup->mouseDown.connect(onMousePressed(popup));

            QTimer::singleShot(5000, [popup] {
                popup->hide();
                popup->deleteLater();
            });

            return Success;
        })
        .onError([channelName, bottomText,
                  onMousePressed](auto result) -> bool {
            auto *popup = new NotificationPopup();
            popup->updatePosition();
            popup->setImageAndText(getResources().icon,
                                   QString(channelName + " just went live!"),
                                   bottomText);
            popup->show();
            popup->mouseDown.connect(onMousePressed(popup));

            QTimer::singleShot(5000, [popup] {
                popup->hide();
                popup->deleteLater();
            });
            return false;
        })
        .execute();
}
}  // namespace chatterino
