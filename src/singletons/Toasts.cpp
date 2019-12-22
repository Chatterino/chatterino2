#include "Toasts.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "libsnore/snore.h"
#include "libsnore/snore_p.h"
#include "libsnore/utils.h"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "util/StreamLink.hpp"
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
    : app(QStringLiteral("Chatterino2"), Snore::Icon::defaultIcon())
{
    Snore::SnoreCore &instance = Snore::SnoreCore::instance();
    instance.loadPlugins(Snore::SnorePlugin::Backend);
    Snore::SnoreCore::instance().registerApplication(app);
    qDebug() << Snore::SnoreCore::instance().pluginNames(
        Snore::SnorePlugin::Backend);

    auto notifyCallback = [this](Snore::Notification notification) {
        switch (
            static_cast<ToastReaction>(getSettings()->openFromToast.getValue()))
        {
            case ToastReaction::OpenInBrowser:
                QDesktopServices::openUrl(QUrl(
                    "https://twitch.tv/" + notification.title().split(' ')[0]));
                break;
            case ToastReaction::OpenInPlayer:
                QDesktopServices::openUrl(
                    QUrl("https://player.twitch.tv/?channel=" +
                         notification.title().split(' ')[0]));
                break;
            case ToastReaction::OpenInStreamlink:
                openStreamlinkForChannel(notification.title().split(' ')[0]);
                break;
        }
    };
    QObject::connect(&instance, &Snore::SnoreCore::actionInvoked,
                     notifyCallback);
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
    Snore::SnoreCore &instance = Snore::SnoreCore::instance();
    QString bottomText = "";
    if (static_cast<ToastReaction>(getSettings()->openFromToast.getValue()) !=
        ToastReaction::DontOpen)
    {
        QString mode =
            Toasts::findStringFromReaction(getSettings()->openFromToast)
                .toLower();

        bottomText = "Click here to " + mode;
    }

    NetworkRequest::twitchRequest(url)
        .onSuccess(
            [this, channelName, bottomText, &instance](auto result) -> Outcome {
                const auto data = result.getData();

                qDebug() << instance.pluginNames(Snore::SnorePlugin::Backend);
                QPixmap avatar;
                avatar.loadFromData(data);

                instance.broadcastNotification(Snore::Notification(
                    app, app.defaultAlert(), channelName + " just went live!",
                    bottomText, Snore::Icon(avatar)));
                return Success;
            })
        .onError(
            [this, channelName, bottomText, &instance](auto result) -> bool {
                instance.broadcastNotification(Snore::Notification(
                    app, app.defaultAlert(), channelName + " just went live!",
                    bottomText, app.icon()));
                return false;
            })
        .execute();
}
}  // namespace chatterino
