#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationModel.hpp"

#include <wintoastlib.h>

#include <QDesktopServices>
#include <QDir>
#include <QUrl>

namespace chatterino {

void NotificationController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
    for (const QString &channelName : this->twitchSetting_.getValue()) {
        this->twitchVector.appendItem(channelName);
    }

    this->twitchVector.delayedItemsChanged.connect([this] {  //
        this->twitchSetting_.setValue(this->twitchVector.getVector());
    });

    for (const QString &channelName : this->mixerSetting_.getValue()) {
        this->mixerVector.appendItem(channelName);
    }

    this->mixerVector.delayedItemsChanged.connect([this] {  //
        this->mixerSetting_.setValue(this->mixerVector.getVector());
    });
}

void NotificationController::updateChannelNotification(
    const QString &channelName, Platform p)
{
    if (p == Platform::Twitch) {
        if (isChannelNotified(channelName, Platform::Twitch)) {
            removeChannelNotification(channelName, twitchVector);
        } else {
            addChannelNotification(channelName, twitchVector);
        }
    } else if (p == Platform::Mixer) {
        if (isChannelNotified(channelName, Platform::Mixer)) {
            removeChannelNotification(channelName, mixerVector);
        } else {
            addChannelNotification(channelName, mixerVector);
        }
    }
}

bool NotificationController::isChannelNotified(const QString &channelName,
                                               Platform p)
{
    /*
    for (std::vector<int>::size_type i = 0;
         i != notificationVector.getVector().size(); i++) {
        qDebug() << notificationVector.getVector()[i]
                 << " vector to the left channel to the right " << channelName
                 << " vectorsize:" << notificationVector.getVector().size();
    }
    */
    // qDebug() << channelName << " channel and now i: " << i;
    if (p == Platform::Twitch) {
        for (std::vector<int>::size_type i = 0;
             i != twitchVector.getVector().size(); i++) {
            if (twitchVector.getVector()[i] == channelName) {
                return true;
            }
        }
    } else if (p == Platform::Mixer) {
        for (std::vector<int>::size_type i = 0;
             i != mixerVector.getVector().size(); i++) {
            if (mixerVector.getVector()[i] == channelName) {
                return true;
            }
        }
    }
    return false;
}
/*
class CustomHandler : public WinToastLib::IWinToastHandler
{
public:
    void toastActivated() const
    {
        std::wcout << L"The user clicked in this toast" << std::endl;
    }

    void toastActivated(int actionIndex) const
    {
        std::wcout << L"The user clicked on button #" << actionIndex
                   << L" in this toast" << std::endl;
        QDesktopServices::openUrl(
            QUrl("http://www.google.com", QUrl::TolerantMode));
    }

    void toastFailed() const
    {
        std::wcout << L"Error showing current toast" << std::endl;
    }
    void toastDismissed(WinToastDismissalReason state) const
    {
        switch (state) {
            case UserCanceled:
                std::wcout << L"The user dismissed this toast" << std::endl;
                break;
            case ApplicationHidden:
                std::wcout << L"The application hid the toast using "
                              L"ToastNotifier.hide()"
                           << std::endl;
                break;
            case TimedOut:
                std::wcout << L"The toast has timed out" << std::endl;
                break;
            default:
                std::wcout << L"Toast not activated" << std::endl;
                break;
        }
    }
};
*/
void NotificationController::addChannelNotification(
    const QString &channelName, UnsortedSignalVector<QString> &vector)
{
    vector.appendItem(channelName);

    if (WinToastLib::WinToast::isCompatible()) {
        // QDir dir;
        // qDebug() << "NaM" << dir.absolutePath();
        /*

        WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(
            WinToastLib::WinToastTemplate::ImageAndText02);
        templ.setTextField(L"Your favorite streamer has gone live!",
                           WinToastLib::WinToastTemplate::FirstLine);
        templ.setTextField(L"NaM!", WinToastLib::WinToastTemplate::SecondLine);
        // templ.setExpiration(10);
        // templ.setImagePath();
        // templ.setAudioOption(WinToastLib::WinToastTemplate::Silent);
        // templ.setAudioPath(L"C:/ping2.wav");
        WinToastLib::WinToast::instance()->setAppName(L"Chatterino2");
        WinToastLib::WinToast::instance()->setAppUserModelId(
            WinToastLib::WinToast::configureAUMI(
                L"mohabouje", L"wintoast", L"wintoastexample", L"20161006"));
        WinToastLib::WinToast::instance()->initialize();
        WinToastLib::WinToast::instance()->showToast(templ,
                                                     new CustomHandler());
                                                     */
    }
}

void NotificationController::removeChannelNotification(
    const QString &channelName, UnsortedSignalVector<QString> &vector)
{
    for (std::vector<int>::size_type i = 0; i != vector.getVector().size();
         i++) {
        if (vector.getVector()[i] == channelName) {
            vector.removeItem(i);
            i--;
        }
    }
}

NotificationModel *NotificationController::createModel(QObject *parent,
                                                       Platform p)
{
    NotificationModel *model = new NotificationModel(parent);
    if (p == Platform::Twitch) {
        model->init(&this->twitchVector);
    } else if (p == Platform::Mixer) {
        model->init(&this->mixerVector);
    }
    return model;
}

}  // namespace chatterino
