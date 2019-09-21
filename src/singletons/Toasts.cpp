#include "Toasts.hpp"

#include "Application.hpp"
#include "common/DownloadManager.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "util/StreamLink.hpp"
#include "widgets/helper/CommonTexts.hpp"

#ifdef Q_OS_WIN

#    include <wintoastlib.h>

#endif

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

bool Toasts::isEnabled()
{
#ifdef Q_OS_WIN
    return WinToastLib::WinToast::isCompatible() &&
           getSettings()->notificationToast;
#else
    return false;
#endif
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

void Toasts::sendChannelNotification(const QString &channelName, Platform p)
{
#ifdef Q_OS_WIN
    auto sendChannelNotification = [this, channelName, p] {
        this->sendWindowsNotification(channelName, p);
    };
#else
    auto sendChannelNotification = [] {
        // Unimplemented for OSX and Linux
    };
#endif
    // Fetch user profile avatar
    if (p == Platform::Twitch)
    {
        QFileInfo check_file(getPaths()->twitchProfileAvatars + "/twitch/" +
                             channelName + ".png");
        if (check_file.exists() && check_file.isFile())
        {
            sendChannelNotification();
        }
        else
        {
            this->fetchChannelAvatar(
                channelName,
                [channelName, sendChannelNotification](QString avatarLink) {
                    DownloadManager *manager = new DownloadManager();
                    manager->setFile(avatarLink, channelName);
                    manager->connect(manager,
                                     &DownloadManager::downloadComplete,
                                     sendChannelNotification);
                });
        }
    }
}

#ifdef Q_OS_WIN

class CustomHandler : public WinToastLib::IWinToastHandler
{
private:
    QString channelName_;
    Platform platform_;

public:
    CustomHandler(QString channelName, Platform p)
        : channelName_(channelName)
        , platform_(p)
    {
    }
    void toastActivated() const
    {
        QString link;
        auto toastReaction =
            static_cast<ToastReaction>(getSettings()->openFromToast.getValue());

        switch (toastReaction)
        {
            case ToastReaction::OpenInBrowser:
                if (platform_ == Platform::Twitch)
                {
                    link = "http://www.twitch.tv/" + channelName_;
                }
                QDesktopServices::openUrl(QUrl(link));
                break;
            case ToastReaction::OpenInPlayer:
                if (platform_ == Platform::Twitch)
                {
                    link = "https://player.twitch.tv/?channel=" + channelName_;
                }
                QDesktopServices::openUrl(QUrl(link));
                break;
            case ToastReaction::OpenInStreamlink:
            {
                openStreamlinkForChannel(channelName_);
                break;
            }
                // the fourth and last option is "don't open"
                // in this case obviously nothing should happen
        }
    }

    void toastActivated(int actionIndex) const
    {
    }

    void toastFailed() const
    {
    }

    void toastDismissed(WinToastDismissalReason state) const
    {
    }
};

void Toasts::sendWindowsNotification(const QString &channelName, Platform p)
{
    WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(
        WinToastLib::WinToastTemplate::ImageAndText03);
    QString str = channelName + " is live!";
    std::string utf8_text = str.toUtf8().constData();
    std::wstring widestr = std::wstring(utf8_text.begin(), utf8_text.end());

    templ.setTextField(widestr, WinToastLib::WinToastTemplate::FirstLine);
    if (static_cast<ToastReaction>(getSettings()->openFromToast.getValue()) !=
        ToastReaction::DontOpen)
    {
        QString mode =
            Toasts::findStringFromReaction(getSettings()->openFromToast);
        mode = mode.toLower();

        templ.setTextField(L"Click here to " + mode.toStdWString(),
                           WinToastLib::WinToastTemplate::SecondLine);
    }

    QString Path;
    if (p == Platform::Twitch)
    {
        Path = getPaths()->twitchProfileAvatars + "/twitch/" + channelName +
               ".png";
    }
    std::string temp_Utf8 = Path.toUtf8().constData();
    std::wstring imagePath = std::wstring(temp_Utf8.begin(), temp_Utf8.end());
    templ.setImagePath(imagePath);
    if (getSettings()->notificationPlaySound)
    {
        templ.setAudioOption(
            WinToastLib::WinToastTemplate::AudioOption::Silent);
    }
    WinToastLib::WinToast::instance()->setAppName(L"Chatterino2");
    int mbstowcs(wchar_t * aumi_version, const char *CHATTERINO_VERSION,
                 size_t size);
    std::string(CHATTERINO_VERSION);
    std::wstring aumi_version =
        std::wstring(CHATTERINO_VERSION.begin(), CHATTERINO_VERSION.end());
    WinToastLib::WinToast::instance()->setAppUserModelId(
        WinToastLib::WinToast::configureAUMI(L"", L"Chatterino 2", L"",
                                             aumi_version));
    WinToastLib::WinToast::instance()->initialize();
    WinToastLib::WinToast::instance()->showToast(
        templ, new CustomHandler(channelName, p));
}

#endif

void Toasts::fetchChannelAvatar(const QString channelName,
                                std::function<void(QString)> successCallback)
{
    QString requestUrl("https://api.twitch.tv/kraken/users?login=" +
                       channelName);

    NetworkRequest(requestUrl)

        .authorizeTwitchV5(getDefaultClientID())
        .timeout(30000)
        .onSuccess([successCallback](auto result) mutable -> Outcome {
            auto root = result.parseJson();
            if (!root.value("users").isArray())
            {
                // log("API Error while getting user id, users is not an array");
                successCallback("");
                return Failure;
            }
            auto users = root.value("users").toArray();
            if (users.size() != 1)
            {
                // log("API Error while getting user id, users array size is not
                // 1");
                successCallback("");
                return Failure;
            }
            if (!users[0].isObject())
            {
                // log("API Error while getting user id, first user is not an
                // object");
                successCallback("");
                return Failure;
            }
            auto firstUser = users[0].toObject();
            auto avatar = firstUser.value("logo");
            if (!avatar.isString())
            {
                // log("API Error: while getting user avatar, first user object "
                //    "`avatar` key "
                //    "is not a "
                //    "string");
                successCallback("");
                return Failure;
            }
            successCallback(avatar.toString());
            return Success;
        })
        .execute();
}
}  // namespace chatterino
