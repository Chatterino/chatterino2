#pragma once

#include "singletons/NativeMessaging.hpp"

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
class ILogging;
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
class BttvLiveUpdates;
class FfzEmotes;
class SeventvEmotes;
class SeventvEventAPI;
class ILinkResolver;
class IStreamerMode;
class ITwitchUsers;
namespace pronouns {
    class Pronouns;
}  // namespace pronouns
namespace eventsub {
    class IController;
}  // namespace eventsub

class IApplication
{
public:
    IApplication();
    virtual ~IApplication();

    IApplication(const IApplication &) = delete;
    IApplication(IApplication &&) = delete;
    IApplication &operator=(const IApplication &) = delete;
    IApplication &operator=(IApplication &&) = delete;

    virtual bool isTest() const = 0;

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
    virtual ILogging *getChatLogger() = 0;
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
    virtual BttvLiveUpdates *getBttvLiveUpdates() = 0;
    virtual FfzEmotes *getFfzEmotes() = 0;
    virtual SeventvEmotes *getSeventvEmotes() = 0;
    virtual SeventvEventAPI *getSeventvEventAPI() = 0;
    virtual ILinkResolver *getLinkResolver() = 0;
    virtual IStreamerMode *getStreamerMode() = 0;
    virtual ITwitchUsers *getTwitchUsers() = 0;
    virtual pronouns::Pronouns *getPronouns() = 0;
    virtual eventsub::IController *getEventSub() = 0;
};

class Application : public IApplication
{
    const Paths &paths_;
    const Args &args_;
    int argc_{};
    char **argv_{};

public:
    Application(Settings &_settings, const Paths &paths, const Args &_args,
                Updates &_updates);
    ~Application() override;

    Application(const Application &) = delete;
    Application(Application &&) = delete;
    Application &operator=(const Application &) = delete;
    Application &operator=(Application &&) = delete;

    bool isTest() const override
    {
        return false;
    }

    void initialize(Settings &settings, const Paths &paths);
    void load();
    void save();

    int run();

    friend void test();

private:
    std::unique_ptr<Theme> themes;
    std::unique_ptr<Fonts> fonts;
    const std::unique_ptr<Logging> logging;
    std::unique_ptr<Emotes> emotes;
    std::unique_ptr<AccountController> accounts;
    std::unique_ptr<eventsub::IController> eventSub;
    std::unique_ptr<HotkeyController> hotkeys;
    std::unique_ptr<WindowManager> windows;
    std::unique_ptr<Toasts> toasts;
    std::unique_ptr<ImageUploader> imageUploader;
    std::unique_ptr<SeventvAPI> seventvAPI;
    std::unique_ptr<CrashHandler> crashHandler;
    std::unique_ptr<CommandController> commands;
    std::unique_ptr<NotificationController> notifications;
    std::unique_ptr<HighlightController> highlights;
    std::unique_ptr<TwitchIrcServer> twitch;
    std::unique_ptr<FfzBadges> ffzBadges;
    std::unique_ptr<SeventvBadges> seventvBadges;
    std::unique_ptr<UserDataController> userData;
    std::unique_ptr<ISoundController> sound;
    std::unique_ptr<TwitchLiveController> twitchLiveController;
    std::unique_ptr<PubSub> twitchPubSub;
    std::unique_ptr<TwitchBadges> twitchBadges;
    std::unique_ptr<ChatterinoBadges> chatterinoBadges;
    std::unique_ptr<BttvEmotes> bttvEmotes;
    std::unique_ptr<BttvLiveUpdates> bttvLiveUpdates;
    std::unique_ptr<FfzEmotes> ffzEmotes;
    std::unique_ptr<SeventvEmotes> seventvEmotes;
    std::unique_ptr<SeventvEventAPI> seventvEventAPI;
    std::unique_ptr<ILinkResolver> linkResolver;
    std::unique_ptr<IStreamerMode> streamerMode;
    std::unique_ptr<ITwitchUsers> twitchUsers;
    std::unique_ptr<pronouns::Pronouns> pronouns;
#ifdef CHATTERINO_HAVE_PLUGINS
    std::unique_ptr<PluginController> plugins;
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
    ILogging *getChatLogger() override;
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
    Updates &getUpdates() override;

    BttvEmotes *getBttvEmotes() override;
    BttvLiveUpdates *getBttvLiveUpdates() override;
    FfzEmotes *getFfzEmotes() override;
    SeventvEmotes *getSeventvEmotes() override;
    SeventvEventAPI *getSeventvEventAPI() override;
    pronouns::Pronouns *getPronouns() override;
    eventsub::IController *getEventSub() override;

    ILinkResolver *getLinkResolver() override;
    IStreamerMode *getStreamerMode() override;
    ITwitchUsers *getTwitchUsers() override;

private:
    void initBttvLiveUpdates();
    void initSeventvEventAPI();
    void initNm(const Paths &paths);

    NativeMessagingServer nmServer;
    Updates &updates;

    bool initialized{false};
};

IApplication *getApp();

/// Might return `nullptr` if the app is being destroyed
IApplication *tryGetApp();

}  // namespace chatterino
