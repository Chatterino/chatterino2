#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"
/*
#ifdef Q_OS_WIN

#include "wintoastlib.h"

#endif
*/
namespace chatterino {

enum class Platform : uint8_t;

class Toasts final : public Singleton
{
public:
    void sendChannelNotification(const QString &channelName, Platform p);

    static bool isEnabled();

private:
    void sendWindowsNotification(const QString &channelName, Platform p);
    void sendActualWindowsNotification(const QString &channelName, Platform p);
    static void fetchChannelAvatar(
        const QString channelName,
        std::function<void(QString)> successCallback);
};
}  // namespace chatterino
