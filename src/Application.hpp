#pragma once

#include "common/Singleton.hpp"
#include "singletons/NativeMessaging.hpp"

#include <pajlada/signals.hpp>
#include <pajlada/signals/signal.hpp>
#include <QApplication>

#include <cassert>
#include <memory>

namespace chatterino {

class Args;
class TwitchIrcServer;
class ITwitchIrcServer;
class PubSub;
class Updates;

class CommandController;
class AccountController;
class NotificationController;
class HighlightController;
class HotkeyController;
class IUserDataController;
class UserDataController;
class ISoundController;
class SoundController;
class ITwitchLiveController;
class TwitchLiveController;
class TwitchBadges;
#ifdef CHATTERINO_HAVE_PLUGINS
class PluginController;
#endif

class Theme;
class WindowManager;
class Logging;
class Paths;
class Emotes;
class IEmotes;
class Settings;
class Fonts;
class Toasts;
class ChatterinoBadges;
class FfzBadges;
class SeventvBadges;
class ImageUploader;
class SeventvAPI;
class CrashHandler;

class CApplication
{
public:
    CApplication();
    virtual ~CApplication() = default;
    CApplication(const CApplication &) = delete;
    CApplication(CApplication &&) = delete;
    CApplication &operator=(const CApplication &) = delete;
    CApplication &operator=(CApplication &&) = delete;

    static CApplication *instance;

    virtual const Paths &getPaths() = 0;
    virtual const Args &getArgs() = 0;
};

class IApplication : public CApplication
{
public:
    IApplication();
    virtual ~IApplication() = default;

    static IApplication *instance;

    virtual Theme *getThemes() = 0;
    virtual Fonts *getFonts() = 0;
    virtual IEmotes *getEmotes() = 0;
    virtual AccountController *getAccounts() = 0;
    virtual HotkeyController *getHotkeys() = 0;
    virtual WindowManager *getWindows() = 0;
    virtual Toasts *getToasts() = 0;
    virtual CrashHandler *getCrashHandler() = 0;
    virtual CommandController *getCommands() = 0;
    virtual HighlightController *getHighlights() = 0;
    virtual NotificationController *getNotifications() = 0;
    virtual ITwitchIrcServer *getTwitch() = 0;
    virtual PubSub *getTwitchPubSub() = 0;
    virtual Logging *getChatLogger() = 0;
    virtual ChatterinoBadges *getChatterinoBadges() = 0;
    virtual FfzBadges *getFfzBadges() = 0;
    virtual SeventvBadges *getSeventvBadges() = 0;
    virtual IUserDataController *getUserData() = 0;
    virtual ISoundController *getSound() = 0;
    virtual ITwitchLiveController *getTwitchLiveController() = 0;
    virtual TwitchBadges *getTwitchBadges() = 0;
    virtual ImageUploader *getImageUploader() = 0;
    virtual SeventvAPI *getSeventvAPI() = 0;
    virtual Updates &getUpdates() = 0;
};

class Application : public IApplication
{
    const Paths &paths_;
    const Args &args_;
    std::vector<std::unique_ptr<Singleton>> singletons_;
    int argc_{};
    char **argv_{};

public:
    static Application *instance;

    Application(Settings &_settings, const Paths &paths, const Args &_args,
                Updates &_updates);
    ~Application() override;

    Application(const Application &) = delete;
    Application(Application &&) = delete;
    Application &operator=(const Application &) = delete;
    Application &operator=(Application &&) = delete;

    /**
     * In the interim, before we remove _exit(0); from RunGui.cpp,
     * this will destroy things we know can be destroyed
     */
    void fakeDtor();

    void initialize(Settings &settings, const Paths &paths);
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
    ImageUploader *const imageUploader{};
    SeventvAPI *const seventvAPI{};
    CrashHandler *const crashHandler{};

    CommandController *const commands{};
    NotificationController *const notifications{};
    HighlightController *const highlights{};
    TwitchIrcServer *const twitch{};
    ChatterinoBadges *const chatterinoBadges{};
    FfzBadges *const ffzBadges{};
    SeventvBadges *const seventvBadges{};
    UserDataController *const userData{};
    ISoundController *const sound{};

private:
    TwitchLiveController *const twitchLiveController{};
    std::unique_ptr<PubSub> twitchPubSub;
    std::unique_ptr<TwitchBadges> twitchBadges;
    const std::unique_ptr<Logging> logging;

public:
#ifdef CHATTERINO_HAVE_PLUGINS
    PluginController *const plugins{};
#endif

    const Paths &getPaths() override
    {
        return this->paths_;
    }
    const Args &getArgs() override
    {
        return this->args_;
    }
    Theme *getThemes() override
    {
        return this->themes;
    }
    Fonts *getFonts() override
    {
        return this->fonts;
    }
    IEmotes *getEmotes() override;
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
    CrashHandler *getCrashHandler() override
    {
        return this->crashHandler;
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
    ITwitchIrcServer *getTwitch() override;
    PubSub *getTwitchPubSub() override;
    Logging *getChatLogger() override;
    ChatterinoBadges *getChatterinoBadges() override
    {
        return this->chatterinoBadges;
    }
    FfzBadges *getFfzBadges() override
    {
        return this->ffzBadges;
    }
    SeventvBadges *getSeventvBadges() override
    {
        return this->seventvBadges;
    }
    IUserDataController *getUserData() override;
    ISoundController *getSound() override;
    ITwitchLiveController *getTwitchLiveController() override;
    TwitchBadges *getTwitchBadges() override;
    ImageUploader *getImageUploader() override
    {
        return this->imageUploader;
    }
    SeventvAPI *getSeventvAPI() override
    {
        return this->seventvAPI;
    }
    Updates &getUpdates() override
    {
        return this->updates;
    }

    pajlada::Signals::NoArgSignal streamerModeChanged;

private:
    void addSingleton(Singleton *singleton);
    void initPubSub();
    void initBttvLiveUpdates();
    void initSeventvEventAPI();
    void initNm(const Paths &paths);

    template <typename T,
              typename = std::enable_if_t<std::is_base_of<Singleton, T>::value>>
    T &emplace()
    {
        auto t = new T;
        this->singletons_.push_back(std::unique_ptr<T>(t));
        return *t;
    }

    template <typename T,
              typename = std::enable_if_t<std::is_base_of<Singleton, T>::value>>
    T &emplace(T *t)
    {
        this->singletons_.push_back(std::unique_ptr<T>(t));
        return *t;
    }

    NativeMessagingServer nmServer{};
    Updates &updates;
};

Application *getApp();

// Get an interface version of the Application class - should be preferred when possible for new code
IApplication *getIApp();

/// Gets a subset of the Application class that is safe to use outside of the GUI thread.
inline CApplication *getCApp()
{
    assert(CApplication::instance != nullptr);

    return CApplication::instance;
}

}  // namespace chatterino
