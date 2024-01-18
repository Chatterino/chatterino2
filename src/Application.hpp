#pragma once

#include "common/Singleton.hpp"
#include "debug/AssertInGuiThread.hpp"
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

class IApplication
{
public:
    IApplication();
    virtual ~IApplication() = default;

    static IApplication *instance;

    virtual const Paths &getPaths() = 0;
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
        assertInGuiThread();

        return this->themes;
    }
    Fonts *getFonts() override
    {
        assertInGuiThread();

        return this->fonts;
    }
    IEmotes *getEmotes() override;
    AccountController *getAccounts() override
    {
        assertInGuiThread();

        return this->accounts;
    }
    HotkeyController *getHotkeys() override
    {
        assertInGuiThread();

        return this->hotkeys;
    }
    WindowManager *getWindows() override
    {
        assertInGuiThread();

        return this->windows;
    }
    Toasts *getToasts() override
    {
        assertInGuiThread();

        return this->toasts;
    }
    CrashHandler *getCrashHandler() override
    {
        assertInGuiThread();

        return this->crashHandler;
    }
    CommandController *getCommands() override
    {
        assertInGuiThread();

        return this->commands;
    }
    NotificationController *getNotifications() override
    {
        assertInGuiThread();

        return this->notifications;
    }
    HighlightController *getHighlights() override
    {
        assertInGuiThread();

        return this->highlights;
    }
    ITwitchIrcServer *getTwitch() override;
    PubSub *getTwitchPubSub() override;
    Logging *getChatLogger() override;
    ChatterinoBadges *getChatterinoBadges() override
    {
        assertInGuiThread();

        return this->chatterinoBadges;
    }
    FfzBadges *getFfzBadges() override
    {
        assertInGuiThread();

        return this->ffzBadges;
    }
    SeventvBadges *getSeventvBadges() override
    {
        assertInGuiThread();

        return this->seventvBadges;
    }
    IUserDataController *getUserData() override;
    ISoundController *getSound() override;
    ITwitchLiveController *getTwitchLiveController() override;
    TwitchBadges *getTwitchBadges() override;
    ImageUploader *getImageUploader() override
    {
        assertInGuiThread();

        return this->imageUploader;
    }
    SeventvAPI *getSeventvAPI() override
    {
        assertInGuiThread();

        return this->seventvAPI;
    }
    Updates &getUpdates() override
    {
        assertInGuiThread();

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

}  // namespace chatterino
