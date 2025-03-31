#pragma once

#include <pajlada/settings/setting.hpp>
#include <QString>

#include <cstdint>

namespace chatterino {

enum class Platform : uint8_t;

enum class ToastReaction {
    OpenInBrowser = 0,
    OpenInPlayer = 1,
    OpenInStreamlink = 2,
    DontOpen = 3
};

class Toasts final
{
public:
    ~Toasts();

    void sendChannelNotification(const QString &channelName,
                                 const QString &channelTitle, Platform p);
    static QString findStringFromReaction(const ToastReaction &reaction);
    static QString findStringFromReaction(
        const pajlada::Settings::Setting<int> &reaction);

    static bool isEnabled();

private:
#ifdef Q_OS_WIN
    void ensureInitialized();
    void sendWindowsNotification(const QString &channelName,
                                 const QString &channelTitle, Platform p);

    bool initialized_ = false;
#endif
};
}  // namespace chatterino
