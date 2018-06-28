#pragma once

#include "singletons/IrcManager.hpp"
#include "singletons/Resources.hpp"

#include <QApplication>

namespace chatterino {

class TwitchServer;
class PubSub;

class CommandController;
class HighlightController;
class IgnoreController;
class TaggedUsersController;
class AccountController;
class ModerationActions;

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

    PathManager *paths = nullptr;
    ThemeManager *themes = nullptr;
    WindowManager *windows = nullptr;
    LoggingManager *logging = nullptr;
    CommandController *commands = nullptr;
    HighlightController *highlights = nullptr;
    IgnoreController *ignores = nullptr;
    TaggedUsersController *taggedUsers = nullptr;
    AccountController *accounts = nullptr;
    EmoteManager *emotes = nullptr;
    NativeMessagingManager *nativeMessaging = nullptr;
    SettingManager *settings = nullptr;
    FontManager *fonts = nullptr;
    ResourceManager *resources = nullptr;
    ModerationActions *moderationActions = nullptr;

    /// Provider-specific
    struct {
        TwitchServer *server = nullptr;
        PubSub *pubsub = nullptr;
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
