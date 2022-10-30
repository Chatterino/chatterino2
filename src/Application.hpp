#pragma once

#include <QApplication>
#include <memory>

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "singletons/NativeMessaging.hpp"

namespace chatterino {

class TwitchIrcServer;
class PubSub;

class CommandController;
class AccountController;
class NotificationController;
class HighlightController;
class HotkeyController;

class Theme;
class WindowManager;
class Logging;
class Paths;
class AccountManager;
class Emotes;
class Settings;
class Fonts;
class Toasts;
class ChatterinoBadges;
class FfzBadges;
class SeventvBadges;

class IApplication
{
public:
    IApplication();
    virtual ~IApplication() = default;

    static IApplication *instance;

    virtual Theme *getThemes() = 0;
    virtual Fonts *getFonts() = 0;
    virtual Emotes *getEmotes() = 0;
    virtual AccountController *getAccounts() = 0;
    virtual HotkeyController *getHotkeys() = 0;
    virtual WindowManager *getWindows() = 0;
    virtual Toasts *getToasts() = 0;
    virtual CommandController *getCommands() = 0;
    virtual HighlightController *getHighlights() = 0;
    virtual NotificationController *getNotifications() = 0;
    virtual TwitchIrcServer *getTwitch() = 0;
    virtual ChatterinoBadges *getChatterinoBadges() = 0;
    virtual FfzBadges *getFfzBadges() = 0;
};

class Application : public IApplication
{
    std::vector<std::unique_ptr<Singleton>> singletons_;
    int argc_;
    char **argv_;

public:
    static Application *instance;

    Application(Settings &settings, Paths &paths);

    void initialize(Settings &settings, Paths &paths);
    void load();
    void save();

    int run(QApplication &qtApp);

    friend void test();

    Theme *const themes{};
    Fonts *const fonts{};
    Emotes *const emotes{};
    AccountController *const accounts{};
    HotkeyController *const hotkeys{};
    WindowManager *const windows{};
    Toasts *const toasts{};

    CommandController *const commands{};
    NotificationController *const notifications{};
    HighlightController *const highlights{};
    TwitchIrcServer *const twitch{};
    ChatterinoBadges *const chatterinoBadges{};
    FfzBadges *const ffzBadges{};
    SeventvBadges *const seventvBadges{};

    /*[[deprecated]]*/ Logging *const logging{};

    Theme *getThemes() override
    {
        return this->themes;
    }
    Fonts *getFonts() override
    {
        return this->fonts;
    }
    Emotes *getEmotes() override
    {
        return this->emotes;
    }
    AccountController *getAccounts() override
    {
        return this->accounts;
    }
    HotkeyController *getHotkeys() override
    {
        return this->hotkeys;
    }
    WindowManager *getWindows() override
    {
        return this->windows;
    }
    Toasts *getToasts() override
    {
        return this->toasts;
    }
    CommandController *getCommands() override
    {
        return this->commands;
    }
    NotificationController *getNotifications() override
    {
        return this->notifications;
    }
    HighlightController *getHighlights() override
    {
        return this->highlights;
    }
    TwitchIrcServer *getTwitch() override
    {
        return this->twitch;
    }
    ChatterinoBadges *getChatterinoBadges() override
    {
        return this->chatterinoBadges;
    }
    FfzBadges *getFfzBadges() override
    {
        return this->ffzBadges;
    }

private:
    void addSingleton(Singleton *singleton);
    void initPubSub();
    void initNm(Paths &paths);

    template <typename T,
              typename = std::enable_if_t<std::is_base_of<Singleton, T>::value>>
    T &emplace()
    {
        auto t = new T;
        this->singletons_.push_back(std::unique_ptr<T>(t));
        return *t;
    }

    NativeMessagingServer nmServer{};
};

Application *getApp();

// Get an interface version of the Application class - should be preferred when possible for new code
IApplication *getIApp();

}  // namespace chatterino
