#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"

#include "libsnore/snore.h"

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
    Toasts();

    void sendChannelNotification(const QString &channelName, Platform p);
    static QString findStringFromReaction(const ToastReaction &reaction);
    static QString findStringFromReaction(
        const pajlada::Settings::Setting<int> &reaction);
    static std::map<ToastReaction, QString> reactionToString;

    void sendToastMessage(const QString &channelName);

private:
    Snore::Application app;
    void actuallySendToastMessage(const QUrl &url, const QString &channelName);
};
}  // namespace chatterino
