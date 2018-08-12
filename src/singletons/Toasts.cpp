#include "Toasts.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"

#ifdef Q_OS_WIN

#include <wintoastlib.h>

#endif

#include <QDesktopServices>
#include <QUrl>

#include <cstdlib>

namespace chatterino {

/*
Toasts::Toasts()
{
}
*/
/*
void Toasts::initialize(Settings &settings, Paths &paths)
{
    getApp()->twitch2->forEachChannel([this](ChannelPtr chn) {
        auto twchn = dynamic_cast<TwitchChannel *>(chn.get());
        twchn->liveStatusChanged.connect([twchn, this]() {
            const auto streamStatus = twchn->accessStreamStatus();
            if (streamStatus->live) {
                // is live
                if (getApp()->notifications->isChannelNotified(
                        twchn->getName()) &&
                    !wasChannelLive(twchn->getName())) {
                    sendChannelNotification(twchn->getName());
                }
                updateLiveChannels(twchn->getName());
            } else {
                // is Offline
                removeFromLiveChannels(twchn->getName());
            }
        });
    });
}

void Toasts::updateLiveChannels(const QString &channelName)
{
    if (!wasChannelLive(channelName)) {
        std::lock_guard<std::mutex> lock(mutex_);
        liveChannels.push_back(channelName);
    }
}

void Toasts::removeFromLiveChannels(const QString &channelName)
{
    if (wasChannelLive(channelName)) {
        std::lock_guard<std::mutex> lock(mutex_);
        liveChannels.erase(
            std::find(liveChannels.begin(), liveChannels.end(), channelName));
    }
}

bool Toasts::wasChannelLive(const QString &channelName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &str : liveChannels) {
        if (str == channelName) {
            return true;
        }
    }
    return false;
}
*/
bool Toasts::isEnabled()
{
    return WinToastLib::WinToast::isCompatible() &&
           getApp()->settings->notificationToast;
}

void Toasts::sendChannelNotification(const QString &channelName, Platform p)
{
#ifdef Q_OS_WIN

    sendWindowsNotification(channelName, p);
    return;

#endif
    // OSX

    // LINUX
}

#ifdef Q_OS_WIN

class CustomHandler : public WinToastLib::IWinToastHandler
{
public:
    void toastActivated() const
    {
        std::wcout << L"The user clicked in this toast" << std::endl;
        QString link = "http://www.google.com";
        QDesktopServices::openUrl(QUrl(link));
    }

    void toastActivated(int actionIndex) const
    {
        // std::wcout << L"The user clicked on button #" << actionIndex
        //           << L" in this toast" << std::endl;
    }

    void toastFailed() const
    {
        // std::wcout << L"Error showing current toast" << std::endl;
    }
    void toastDismissed(WinToastDismissalReason state) const
    {
        switch (state) {
            case UserCanceled:
                // std::wcout << L"The user dismissed this toast" << std::endl;
                break;
            case ApplicationHidden:
                /*
                    std::wcout << L"The application hid the toast using "
                                  L"ToastNotifier.hide()"
                               << std::endl;
                               */
                break;
            case TimedOut:
                // std::wcout << L"The toast has timed out" << std::endl;
                break;
            default:
                // std::wcout << L"Toast not activated" << std::endl;
                break;
        }
    }
};

void Toasts::sendWindowsNotification(const QString &channelName, Platform p)
{
    WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(
        WinToastLib::WinToastTemplate::ImageAndText02);
    QString str = channelName + " has just gone live!";
    std::string utf8_text = str.toUtf8().constData();
    std::wstring widestr = std::wstring(utf8_text.begin(), utf8_text.end());

    templ.setTextField(widestr, WinToastLib::WinToastTemplate::FirstLine);
    templ.setTextField(L"Click here to open in browser",
                       WinToastLib::WinToastTemplate::SecondLine);
    WinToastLib::WinToast::instance()->setAppName(L"Chatterino2");
    int mbstowcs(wchar_t * aumi_version, const char *CHATTERINO_VERSION,
                 size_t size);
    std::string(CHATTERINO_VERSION);
    std::wstring aumi_version =
        std::wstring(CHATTERINO_VERSION.begin(), CHATTERINO_VERSION.end());
    // int mbstowcs(wchar_t *out, const char *in, size_t size);
    /*
std::wstring aumi_version =
std::wstring(std::string(CHATTERINO_VERSION).begin(),
        std::string(CHATTERINO_VERSION).end());*/
    WinToastLib::WinToast::instance()->setAppUserModelId(
        WinToastLib::WinToast::configureAUMI(L"", L"Chatterino 2", L"",
                                             aumi_version));
    WinToastLib::WinToast::instance()->initialize();
    WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
}
#endif

}  // namespace chatterino
