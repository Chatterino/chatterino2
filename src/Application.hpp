#pragma once

#include "common/Singleton.hpp"
#include "singletons/NativeMessaging.hpp"

#include <pajlada/signals.hpp>
#include <pajlada/signals/signal.hpp>
#include <QApplication>

#include <memory>

namespace chatterino {

class Args;
class TwitchIrcServer;
class ITwitchIrcServer;
class PubSub;

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

class IApplication
{
public:
    IApplication();
    virtual ~IApplication() = default;

    static IApplication *instance;

    virtual const Args &getArgs() = 0;
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
    virtual ImageUploader *getImageUploader() = 0;
    virtual SeventvAPI *getSeventvAPI() = 0;
};

class Application : public IApplication
{
    const Args &args_;
    std::vector<std::unique_ptr<Singleton>> singletons_;
    int argc_{};
    char **argv_{};

public:
    static Application *instance;

    Application(Settings &_settings, Paths &_paths, const Args &_args);
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
    const std::unique_ptr<Logging> logging;

public:
#ifdef CHATTERINO_HAVE_PLUGINS
    PluginController *const plugins{};
#endif

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
    ImageUploader *getImageUploader() override
    {
        return this->imageUploader;
    }
    SeventvAPI *getSeventvAPI() override
    {
        return this->seventvAPI;
    }

    pajlada::Signals::NoArgSignal streamerModeChanged;

private:
    void addSingleton(Singleton *singleton);
    void initPubSub();
    void initBttvLiveUpdates();
    void initSeventvEventAPI();
    void initNm(Paths &paths);

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
};

Application *getApp();

// Get an interface version of the Application class - should be preferred when possible for new code
IApplication *getIApp();

}  // namespace chatterino
