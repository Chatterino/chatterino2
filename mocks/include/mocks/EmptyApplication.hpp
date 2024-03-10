#pragma once

#include "Application.hpp"
#include "common/Args.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Updates.hpp"

#include <QTemporaryDir>

namespace chatterino::mock {

class EmptyApplication : public IApplication
{
public:
    EmptyApplication()
        : updates_(this->paths_)
    {
    }

    virtual ~EmptyApplication() = default;

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
        assert(
            false &&
            "EmptyApplication::getThemes was called without being initialized");
        return nullptr;
    }

    Fonts *getFonts() override
    {
        assert(
            false &&
            "EmptyApplication::getFonts was called without being initialized");
        return nullptr;
    }

    IEmotes *getEmotes() override
    {
        assert(
            false &&
            "EmptyApplication::getEmotes was called without being initialized");
        return nullptr;
    }

    AccountController *getAccounts() override
    {
        assert(false && "EmptyApplication::getAccounts was called without "
                        "being initialized");
        return nullptr;
    }

    HotkeyController *getHotkeys() override
    {
        assert(false && "EmptyApplication::getHotkeys was called without being "
                        "initialized");
        return nullptr;
    }

    WindowManager *getWindows() override
    {
        assert(false && "EmptyApplication::getWindows was called without being "
                        "initialized");
        return nullptr;
    }

    Toasts *getToasts() override
    {
        assert(
            false &&
            "EmptyApplication::getToasts was called without being initialized");
        return nullptr;
    }

    CrashHandler *getCrashHandler() override
    {
        assert(false && "EmptyApplication::getCrashHandler was called without "
                        "being initialized");
        return nullptr;
    }

    CommandController *getCommands() override
    {
        assert(false && "EmptyApplication::getCommands was called without "
                        "being initialized");
        return nullptr;
    }

    NotificationController *getNotifications() override
    {
        assert(false && "EmptyApplication::getNotifications was called without "
                        "being initialized");
        return nullptr;
    }

    HighlightController *getHighlights() override
    {
        assert(false && "EmptyApplication::getHighlights was called without "
                        "being initialized");
        return nullptr;
    }

    ITwitchIrcServer *getTwitch() override
    {
        assert(
            false &&
            "EmptyApplication::getTwitch was called without being initialized");
        return nullptr;
    }

    PubSub *getTwitchPubSub() override
    {
        assert(false && "getTwitchPubSub was called without being initialized");
        return nullptr;
    }

    TwitchBadges *getTwitchBadges() override
    {
        assert(false && "getTwitchBadges was called without being initialized");
        return nullptr;
    }

    Logging *getChatLogger() override
    {
        assert(!"getChatLogger was called without being initialized");
        return nullptr;
    }

    IChatterinoBadges *getChatterinoBadges() override
    {
        assert(false && "EmptyApplication::getChatterinoBadges was called "
                        "without being initialized");
        return nullptr;
    }

    FfzBadges *getFfzBadges() override
    {
        assert(false && "EmptyApplication::getFfzBadges was called without "
                        "being initialized");
        return nullptr;
    }

    SeventvBadges *getSeventvBadges() override
    {
        assert(!"getSeventvBadges was called without being initialized");
        return nullptr;
    }

    IUserDataController *getUserData() override
    {
        assert(false && "EmptyApplication::getUserData was called without "
                        "being initialized");
        return nullptr;
    }

    ISoundController *getSound() override
    {
        assert(!"getSound was called without being initialized");
        return nullptr;
    }

    ITwitchLiveController *getTwitchLiveController() override
    {
        assert(false && "EmptyApplication::getTwitchLiveController was called "
                        "without being initialized");
        return nullptr;
    }

    ImageUploader *getImageUploader() override
    {
        assert(false && "EmptyApplication::getImageUploader was called without "
                        "being initialized");
        return nullptr;
    }

    SeventvAPI *getSeventvAPI() override
    {
        return nullptr;
    }

#ifdef CHATTERINO_HAVE_PLUGINS
    PluginController *getPlugins() override
    {
        assert(false && "EmptyApplication::getPlugins was called without "
                        "being initialized");
        return nullptr;
    }
#endif

    Updates &getUpdates() override
    {
        return this->updates_;
    }

    BttvEmotes *getBttvEmotes() override
    {
        assert(false && "EmptyApplication::getBttvEmotes was called without "
                        "being initialized");
        return nullptr;
    }

    FfzEmotes *getFfzEmotes() override
    {
        assert(false && "EmptyApplication::getFfzEmotes was called without "
                        "being initialized");
        return nullptr;
    }

    SeventvEmotes *getSeventvEmotes() override
    {
        assert(false && "EmptyApplication::getSeventvEmotes was called without "
                        "being initialized");
        return nullptr;
    }

    ILinkResolver *getLinkResolver() override
    {
        assert(false && "EmptyApplication::getLinkResolver was called without "
                        "being initialized");
        return nullptr;
    }

    IStreamerMode *getStreamerMode() override
    {
        assert(false && "EmptyApplication::getStreamerMode was called without "
                        "being initialized");
        return nullptr;
    }

protected:
    QTemporaryDir settingsDir;
    Paths paths_;
    Args args_;
    Updates updates_;
};

}  // namespace chatterino::mock
