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
    for (const QString &channelName : this->notificationSetting_.getValue()) {
        this->notificationVector.appendItem(channelName);
    }

    this->notificationVector.delayedItemsChanged.connect([this] {  //
        this->notificationSetting_.setValue(
            this->notificationVector.getVector());
    });
}

void NotificationController::updateChannelNotification(
    const QString &channelName)
{
    if (isChannelNotified(channelName)) {
        removeChannelNotification(channelName);
    } else {
        addChannelNotification(channelName);
    }
}

bool NotificationController::isChannelNotified(const QString &channelName)
{
    for (std::vector<int>::size_type i = 0;
         i != notificationVector.getVector().size(); i++) {
        if (notificationVector.getVector()[i] == channelName) {
            return true;
        }
    }
    return false;
}

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

void NotificationController::addChannelNotification(const QString &channelName)
{
    notificationVector.appendItem(channelName);

    if (WinToastLib::WinToast::isCompatible()) {
        QDir dir;
        qDebug() << "NaM" << dir.absolutePath();
        ;

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
    }
}

void NotificationController::removeChannelNotification(
    const QString &channelName)
{
    for (std::vector<int>::size_type i = 0;
         i != notificationVector.getVector().size(); i++) {
        if (notificationVector.getVector()[i] == channelName) {
            notificationVector.removeItem(i);
            i--;
        }
    }
}

NotificationModel *NotificationController::createModel(QObject *parent)
{
    NotificationModel *model = new NotificationModel(parent);
    model->init(&this->notificationVector);
    return model;
}

}  // namespace chatterino
