#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"

namespace chatterino {

enum class Platform : uint8_t;

class Toasts final : public Singleton
{
public:
    void sendChannelNotification(const QString &channelName, Platform p);

    static bool isEnabled();

private:
    void sendWindowsNotification(const QString &channelName, Platform p);
};
}  // namespace chatterino
