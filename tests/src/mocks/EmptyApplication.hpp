#pragma once

#include "Application.hpp"

using namespace chatterino;

class EmptyApplication : public IApplication
{
public:
    virtual Theme *getThemes() override
    {
        return nullptr;
    }
    virtual Fonts *getFonts() override
    {
        return nullptr;
    }
    virtual IEmotes *getEmotes() override
    {
        return nullptr;
    }
    virtual AccountController *getAccounts() override
    {
        return nullptr;
    }
    virtual HotkeyController *getHotkeys() override
    {
        return nullptr;
    }
    virtual WindowManager *getWindows() override
    {
        return nullptr;
    }
    virtual Toasts *getToasts() override
    {
        return nullptr;
    }
    virtual CommandController *getCommands() override
    {
        return nullptr;
    }
    virtual NotificationController *getNotifications() override
    {
        return nullptr;
    }
    virtual HighlightController *getHighlights() override
    {
        return nullptr;
    }
    virtual ITwitchIrcServer *getTwitch() override
    {
        return nullptr;
    }
    virtual ChatterinoBadges *getChatterinoBadges() override
    {
        return nullptr;
    }
    virtual FfzBadges *getFfzBadges() override
    {
        return nullptr;
    }
    virtual IUserDataController *getUserData() override
    {
        return nullptr;
    }
};
