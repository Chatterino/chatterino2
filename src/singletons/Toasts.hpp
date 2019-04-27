#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"

namespace chatterino {

enum class Platform : uint8_t;

enum class ToastReactions {
    openInBrowser,
    openInPlayer,
    openInStreamlink,
    dontOpen
};

class Toasts final : public Singleton
{
public:
    void sendChannelNotification(const QString &channelName, Platform p);
    static QString findStringFromReaction(const ToastReactions &reaction);
    static ToastReactions findReactionFromString(const QString &string);
    static std::map<ToastReactions, QString> reactionToString;
    static std::map<QString, ToastReactions> stringToReaction;

    static bool isEnabled();

private:
#ifdef Q_OS_WIN
    void sendWindowsNotification(const QString &channelName, Platform p);
#endif

    static void fetchChannelAvatar(
        const QString channelName,
        std::function<void(QString)> successCallback);
};
}  // namespace chatterino
