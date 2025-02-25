#include "singletons/Toasts.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "util/StreamLink.hpp"
#include "widgets/helper/CommonTexts.hpp"

#ifdef Q_OS_WIN
#    include <wintoastlib.h>
#elif defined(CHATTERINO_WITH_LIBNOTIFY)
#    include <libnotify/notify.h>
#endif

#include <QDesktopServices>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringBuilder>
#include <QUrl>

#include <utility>

namespace {

using namespace chatterino;
using namespace literals;

QString avatarFilePath(const QString &channelName)
{
    // TODO: cleanup channel (to be used as a file) and use combinePath
    return getApp()->getPaths().twitchProfileAvatars % '/' % channelName %
           u".png";
}

bool hasAvatarForChannel(const QString &channelName)
{
    QFileInfo avatarFile(avatarFilePath(channelName));
    return avatarFile.exists() && avatarFile.isFile();
}

/// A job that downlaods a twitch avatar and saves it to a file
class AvatarDownloader : public QObject
{
    Q_OBJECT
public:
    AvatarDownloader(const QString &avatarURL, const QString &channelName);

private:
    QNetworkAccessManager manager_;
    QFile file_;
    QNetworkReply *reply_{};

Q_SIGNALS:
    void downloadComplete();
};

}  // namespace

namespace chatterino {

#ifdef Q_OS_WIN
using WinToastLib::WinToast;
using WinToastLib::WinToastTemplate;
#endif

Toasts::~Toasts()
{
#ifdef Q_OS_WIN
    if (this->initialized_)
    {
        WinToast::instance()->clear();
    }
#elif defined(CHATTERINO_WITH_LIBNOTIFY)
    if (this->initialized_)
    {
        notify_uninit();
    }
#endif
}

bool Toasts::isEnabled()
{
    auto enabled = getSettings()->notificationToast &&
                   !(getApp()->getStreamerMode()->isEnabled() &&
                     getSettings()->streamerModeSuppressLiveNotifications);

#ifdef Q_OS_WIN
    enabled = enabled && WinToast::isCompatible();
#endif

    return enabled;
}

QString Toasts::findStringFromReaction(const ToastReaction &reaction)
{
    switch (reaction)
    {
        case ToastReaction::OpenInBrowser:
            return OPEN_IN_BROWSER;
        case ToastReaction::OpenInPlayer:
            return OPEN_PLAYER_IN_BROWSER;
        case ToastReaction::OpenInStreamlink:
            return OPEN_IN_STREAMLINK;
        case ToastReaction::DontOpen:
        default:
            return DONT_OPEN;
    }
}

QString Toasts::findStringFromReaction(
    const pajlada::Settings::Setting<int> &reaction)
{
    static_assert(std::is_same_v<std::underlying_type_t<ToastReaction>, int>);
    int value = reaction;
    return Toasts::findStringFromReaction(static_cast<ToastReaction>(value));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Toasts::sendChannelNotification(const QString &channelName,
                                     const QString &channelTitle)
{
#ifdef Q_OS_WIN
    auto sendChannelNotification = [this, channelName, channelTitle] {
        this->sendWindowsNotification(channelName, channelTitle);
    };
#elif defined(CHATTERINO_WITH_LIBNOTIFY)
    auto sendChannelNotification = [this, channelName, channelTitle] {
        this->sendLibnotify(channelName, channelTitle);
    };
#else
    (void)channelTitle;
    auto sendChannelNotification = [] {
        // Unimplemented for macOS
    };
#endif
    // Fetch user profile avatar
    if (hasAvatarForChannel(channelName))
    {
        sendChannelNotification();
    }
    else
    {
        getHelix()->getUserByName(
            channelName,
            [channelName, sendChannelNotification](const auto &user) {
                // gets deleted when finished
                auto *downloader =
                    new AvatarDownloader(user.profileImageUrl, channelName);
                QObject::connect(downloader,
                                 &AvatarDownloader::downloadComplete,
                                 sendChannelNotification);
            },
            [] {
                // on failure
            });
    }
}

#ifdef Q_OS_WIN

class CustomHandler : public WinToastLib::IWinToastHandler
{
private:
    QString channelName_;

public:
    CustomHandler(QString channelName)
        : channelName_(std::move(channelName))
    {
    }
    void toastActivated() const override
    {
        auto toastReaction =
            static_cast<ToastReaction>(getSettings()->openFromToast.getValue());

        switch (toastReaction)
        {
            case ToastReaction::OpenInBrowser:
                QDesktopServices::openUrl(
                    QUrl(u"https://www.twitch.tv/" % channelName_));
                break;
            case ToastReaction::OpenInPlayer:
                QDesktopServices::openUrl(
                    QUrl(TWITCH_PLAYER_URL.arg(channelName_)));
                break;
            case ToastReaction::OpenInStreamlink: {
                openStreamlinkForChannel(channelName_);
                break;
            }
            case ToastReaction::DontOpen:
                // nothing should happen
                break;
        }
    }

    void toastActivated(int actionIndex) const override
    {
    }

    void toastActivated(const char *response) const override
    {
    }

    void toastFailed() const override
    {
    }

    void toastDismissed(WinToastDismissalReason state) const override
    {
    }
};

void Toasts::ensureInitialized()
{
    if (this->initialized_)
    {
        return;
    }
    this->initialized_ = true;

    auto *instance = WinToast::instance();
    instance->setAppName(L"Chatterino2");
    instance->setAppUserModelId(
        WinToast::configureAUMI(L"", L"Chatterino 2", L"",
                                Version::instance().version().toStdWString()));
    instance->setShortcutPolicy(WinToast::SHORTCUT_POLICY_IGNORE);
    WinToast::WinToastError error{};
    instance->initialize(&error);

    if (error != WinToast::NoError)
    {
        qCDebug(chatterinoNotification)
            << "Failed to initialize WinToast - error:" << error;
    }
}

void Toasts::sendWindowsNotification(const QString &channelName,
                                     const QString &channelTitle)
{
    this->ensureInitialized();

    WinToastTemplate templ(WinToastTemplate::ImageAndText03);
    QString str = channelName % u" is live!";

    templ.setTextField(str.toStdWString(), WinToastTemplate::FirstLine);
    if (static_cast<ToastReaction>(getSettings()->openFromToast.getValue()) !=
        ToastReaction::DontOpen)
    {
        QString mode =
            Toasts::findStringFromReaction(getSettings()->openFromToast);
        mode = mode.toLower();

        templ.setTextField(
            u"%1 \nClick to %2"_s.arg(channelTitle).arg(mode).toStdWString(),
            WinToastTemplate::SecondLine);
    }

    QString avatarPath;
    avatarPath = avatarFilePath(channelName);
    templ.setImagePath(avatarPath.toStdWString());
    if (getSettings()->notificationPlaySound)
    {
        templ.setAudioOption(WinToastTemplate::AudioOption::Silent);
    }

    WinToast::WinToastError error = WinToast::NoError;
    WinToast::instance()->showToast(templ, new CustomHandler(channelName),
                                    &error);
    if (error != WinToast::NoError)
    {
        qCWarning(chatterinoNotification) << "Failed to show toast:" << error;
    }
}

#elif defined(CHATTERINO_WITH_LIBNOTIFY)

void Toasts::ensureInitialized()
{
    if (this->initialized_)
    {
        return;
    }
    auto result = notify_init("chatterino2");

    if (result == 0)
    {
        qCWarning(chatterinoNotification) << "Failed to initialize libnotify";
    }
    this->initialized_ = true;
}

void Toasts::sendLibnotify(const QString &channelName,
                           const QString &channelTitle)
{
    this->ensureInitialized();

    qCDebug(chatterinoNotification) << "sending to libnotify";

    QString str = channelName % u" is live!";

    NotifyNotification *notif = notify_notification_new(
        str.toUtf8().constData(), channelTitle.toUtf8().constData(), nullptr);

    GdkPixbuf *img = gdk_pixbuf_new_from_file(
        avatarFilePath(channelName).toUtf8().constData(), nullptr);
    if (img == nullptr)
    {
        qWarning(chatterinoNotification) << "Failed to load user avatar image";
    }
    else
    {
        notify_notification_set_image_from_pixbuf(notif, img);
        g_object_unref(img);
    }

    notify_notification_show(notif, nullptr);
    g_object_unref(notif);
}
#endif

}  // namespace chatterino

namespace {

AvatarDownloader::AvatarDownloader(const QString &avatarURL,
                                   const QString &channelName)
    : file_(avatarFilePath(channelName))
{
    if (!this->file_.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCWarning(chatterinoNotification)
            << "Failed to open avatar file" << this->file_.errorString();
    }

    this->reply_ = this->manager_.get(QNetworkRequest(avatarURL));

    connect(this->reply_, &QNetworkReply::readyRead, this, [this] {
        this->file_.write(this->reply_->readAll());
    });
    connect(this->reply_, &QNetworkReply::finished, this, [this] {
        if (this->reply_->error() != QNetworkReply::NoError)
        {
            qCWarning(chatterinoNotification)
                << "Failed to download avatar" << this->reply_->errorString();
        }

        if (this->file_.isOpen())
        {
            this->file_.close();
        }
        downloadComplete();
        this->deleteLater();
    });
}

#include "Toasts.moc"

}  // namespace
