#pragma once

#include "common/Singleton.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/NativeMessaging.hpp"

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
class IChatterinoBadges;
class ChatterinoBadges;
class FfzBadges;
class SeventvBadges;
class ImageUploader;
class SeventvAPI;
class CrashHandler;
class BttvEmotes;
class FfzEmotes;
class SeventvEmotes;
class ILinkResolver;
class IStreamerMode;

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
    virtual IChatterinoBadges *getChatterinoBadges() = 0;
    virtual FfzBadges *getFfzBadges() = 0;
    virtual SeventvBadges *getSeventvBadges() = 0;
    virtual IUserDataController *getUserData() = 0;
    virtual ISoundController *getSound() = 0;
    virtual ITwitchLiveController *getTwitchLiveController() = 0;
    virtual TwitchBadges *getTwitchBadges() = 0;
    virtual ImageUploader *getImageUploader() = 0;
    virtual SeventvAPI *getSeventvAPI() = 0;
#ifdef CHATTERINO_HAVE_PLUGINS
    virtual PluginController *getPlugins() = 0;
#endif
    virtual Updates &getUpdates() = 0;
    virtual BttvEmotes *getBttvEmotes() = 0;
    virtual FfzEmotes *getFfzEmotes() = 0;
    virtual SeventvEmotes *getSeventvEmotes() = 0;
    virtual ILinkResolver *getLinkResolver() = 0;
    virtual IStreamerMode *getStreamerMode() = 0;
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

private:
    Theme *const themes{};
    std::unique_ptr<Fonts> fonts{};
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

public:
    TwitchIrcServer *const twitch{};

private:
    FfzBadges *const ffzBadges{};
    SeventvBadges *const seventvBadges{};
    UserDataController *const userData{};
    ISoundController *const sound{};
    TwitchLiveController *const twitchLiveController{};
    std::unique_ptr<PubSub> twitchPubSub;
    std::unique_ptr<TwitchBadges> twitchBadges;
    std::unique_ptr<ChatterinoBadges> chatterinoBadges;
    std::unique_ptr<BttvEmotes> bttvEmotes;
    std::unique_ptr<FfzEmotes> ffzEmotes;
    std::unique_ptr<SeventvEmotes> seventvEmotes;
    const std::unique_ptr<Logging> logging;
    std::unique_ptr<ILinkResolver> linkResolver;
    std::unique_ptr<IStreamerMode> streamerMode;
#ifdef CHATTERINO_HAVE_PLUGINS
    PluginController *const plugins{};
#endif

public:
    const Paths &getPaths() override
    {
        return this->paths_;
    }
    const Args &getArgs() override
    {
        return this->args_;
    }
    Theme *getThemes() override;
    Fonts *getFonts() override;
    IEmotes *getEmotes() override;
    AccountController *getAccounts() override;
    HotkeyController *getHotkeys() override;
    WindowManager *getWindows() override;
    Toasts *getToasts() override;
    CrashHandler *getCrashHandler() override;
    CommandController *getCommands() override;
    NotificationController *getNotifications() override;
    HighlightController *getHighlights() override;
    ITwitchIrcServer *getTwitch() override;
    PubSub *getTwitchPubSub() override;
    Logging *getChatLogger() override;
    FfzBadges *getFfzBadges() override;
    SeventvBadges *getSeventvBadges() override;
    IUserDataController *getUserData() override;
    ISoundController *getSound() override;
    ITwitchLiveController *getTwitchLiveController() override;
    TwitchBadges *getTwitchBadges() override;
    IChatterinoBadges *getChatterinoBadges() override;
    ImageUploader *getImageUploader() override;
    SeventvAPI *getSeventvAPI() override;
#ifdef CHATTERINO_HAVE_PLUGINS
    PluginController *getPlugins() override;
#endif
    Updates &getUpdates() override
    {
        assertInGuiThread();

        return this->updates;
    }

    BttvEmotes *getBttvEmotes() override;
    FfzEmotes *getFfzEmotes() override;
    SeventvEmotes *getSeventvEmotes() override;

    ILinkResolver *getLinkResolver() override;
    IStreamerMode *getStreamerMode() override;

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
