#pragma once

#include <pajlada/settings/setting.hpp>
#include <QString>

namespace chatterino {

enum class ToastReaction {
    OpenInBrowser = 0,
    OpenInPlayer = 1,
    OpenInStreamlink = 2,
    DontOpen = 3,
    OpenInCustomPlayer = 4,
};

class Toasts final
{
public:
    ~Toasts();

    void sendChannelNotification(const QString &channelName,
                                 const QString &channelTitle);
    static QString findStringFromReaction(const ToastReaction &reaction);
    static QString findStringFromReaction(
        const pajlada::Settings::Setting<int> &reaction);

    static bool isEnabled();

private:
#ifdef Q_OS_WIN
    void ensureInitialized();
    void sendWindowsNotification(const QString &channelName,
                                 const QString &channelTitle);

    bool initialized_ = false;
#elif defined(CHATTERINO_WITH_LIBNOTIFY)
    void ensureInitialized();
    void sendLibnotify(const QString &channelName, const QString &channelTitle);

    bool initialized_ = false;
#endif
};
}  // namespace chatterino
