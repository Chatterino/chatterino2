#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"

namespace chatterino {

enum class Platform : uint8_t;

enum class ToastReaction {
    OpenInBrowser = 0,
    OpenInPlayer = 1,
    OpenInStreamlink = 2,
    DontOpen = 3
};

class Toasts final : public Singleton
{
public:
    void sendChannelNotification(const QString &channelName,
                                 const QString &channelTitle, Platform p);
    static QString findStringFromReaction(const ToastReaction &reaction);
    static QString findStringFromReaction(
        const pajlada::Settings::Setting<int> &reaction);
    static std::map<ToastReaction, QString> reactionToString;

    static bool isEnabled();

private:
#ifdef Q_OS_WIN
    void sendWindowsNotification(const QString &channelName,
                                 const QString &channelTitle, Platform p);
#endif
};
}  // namespace chatterino
