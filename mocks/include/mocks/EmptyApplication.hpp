#pragma once

#include "Application.hpp"
#include "common/Args.hpp"

namespace chatterino::mock {

class EmptyApplication : public IApplication
{
public:
    virtual ~EmptyApplication() = default;

    const Args &getArgs() override
    {
        return this->args_;
    }

    Theme *getThemes() override
    {
        return nullptr;
    }

    Fonts *getFonts() override
    {
        return nullptr;
    }

    IEmotes *getEmotes() override
    {
        return nullptr;
    }

    AccountController *getAccounts() override
    {
        return nullptr;
    }

    HotkeyController *getHotkeys() override
    {
        return nullptr;
    }

    WindowManager *getWindows() override
    {
        return nullptr;
    }

    Toasts *getToasts() override
    {
        return nullptr;
    }

    CrashHandler *getCrashHandler() override
    {
        return nullptr;
    }

    CommandController *getCommands() override
    {
        return nullptr;
    }

    NotificationController *getNotifications() override
    {
        return nullptr;
    }

    HighlightController *getHighlights() override
    {
        return nullptr;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return nullptr;
    }

    ChatterinoBadges *getChatterinoBadges() override
    {
        return nullptr;
    }

    FfzBadges *getFfzBadges() override
    {
        return nullptr;
    }

    SeventvBadges *getSeventvBadges() override
    {
        assert(!"getSeventvBadges was called without being initialized");
        return nullptr;
    }

    IUserDataController *getUserData() override
    {
        return nullptr;
    }

    ISoundController *getSound() override
    {
        assert(!"getSound was called without being initialized");
        return nullptr;
    }

    ITwitchLiveController *getTwitchLiveController() override
    {
        return nullptr;
    }

    ImageUploader *getImageUploader() override
    {
        return nullptr;
    }

    SeventvAPI *getSeventvAPI() override
    {
        return nullptr;
    }

private:
    Args args_;
};

}  // namespace chatterino::mock
