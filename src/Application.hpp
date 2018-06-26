#pragma once

#include "singletons/IrcManager.hpp"
#include "singletons/ResourceManager.hpp"

#include <QApplication>

namespace chatterino {

namespace providers {
namespace twitch {

class TwitchServer;
class PubSub;

}  // namespace twitch
}  // namespace providers

namespace controllers {
namespace commands {
class CommandController;
}
namespace highlights {
class HighlightController;
}
namespace ignores {
class IgnoreController;
}
namespace taggedusers {
class TaggedUsersController;
}
namespace accounts {
class AccountController;
}
}  // namespace controllers

namespace singletons {

class ThemeManager;
class WindowManager;
class LoggingManager;
class PathManager;
class AccountManager;
class EmoteManager;
class NativeMessagingManager;
class SettingManager;
class FontManager;
class ResourceManager;

}  // namespace singletons

class Application
{
    Application(int _argc, char **_argv);

public:
    static void instantiate(int argc, char **argv);

    ~Application() = delete;

    void construct();
    void initialize();
    void load();

    int run(QApplication &qtApp);

    friend void test();

    singletons::PathManager *paths = nullptr;
    singletons::ThemeManager *themes = nullptr;
    singletons::WindowManager *windows = nullptr;
    singletons::LoggingManager *logging = nullptr;
    controllers::commands::CommandController *commands = nullptr;
    controllers::highlights::HighlightController *highlights = nullptr;
    controllers::ignores::IgnoreController *ignores = nullptr;
    controllers::taggedusers::TaggedUsersController *taggedUsers = nullptr;
    controllers::accounts::AccountController *accounts = nullptr;
    singletons::EmoteManager *emotes = nullptr;
    singletons::NativeMessagingManager *nativeMessaging = nullptr;
    singletons::SettingManager *settings = nullptr;
    singletons::FontManager *fonts = nullptr;
    singletons::ResourceManager *resources = nullptr;

    /// Provider-specific
    struct {
        providers::twitch::TwitchServer *server = nullptr;
        providers::twitch::PubSub *pubsub = nullptr;
    } twitch;

    void save();

    // Special application mode that only initializes the native messaging host
    static void runNativeMessagingHost();

private:
    int argc;
    char **argv;
};

Application *getApp();

bool appInitialized();

}  // namespace chatterino
