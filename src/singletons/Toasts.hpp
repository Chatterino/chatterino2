#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"

#include <QLayout>

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
    void sendChannelNotification(const QString &channelName, Platform p);
    static QString findStringFromReaction(const ToastReaction &reaction);
    static QString findStringFromReaction(
        const pajlada::Settings::Setting<int> &reaction);
    static std::map<ToastReaction, QString> reactionToString;

    void sendToastMessage(const QString &channelName);

private:
    void actuallySendToastMessage(const QUrl &url, const QString &channelName);
    QHBoxLayout *makeLayout(const QPixmap &image, const QString &text,
                            const QString &bottomText);
};
}  // namespace chatterino
