#pragma once

#include "singletons/Resources.hpp"

#include <QApplication>

namespace chatterino {

class Singleton;

class TwitchServer;
class PubSub;

class CommandController;
class HighlightController;
class IgnoreController;
class TaggedUsersController;
class AccountController;
class ModerationActions;

class Theme;
class WindowManager;
class Logging;
class Paths;
class AccountManager;
class Emotes;
class NativeMessaging;
class Settings;
class Fonts;
class Resources;

class Application
{
    Application(int _argc, char **_argv);

public:
    static void instantiate(int argc_, char **argv_);

    ~Application() = delete;

    void construct();
    void initialize();
    void load();

    int run(QApplication &qtApp);

    friend void test();

    Settings *settings = nullptr;
    Paths *paths = nullptr;

    Theme *themes = nullptr;
    WindowManager *windows = nullptr;
    Logging *logging = nullptr;
    CommandController *commands = nullptr;
    HighlightController *highlights = nullptr;
    IgnoreController *ignores = nullptr;
    TaggedUsersController *taggedUsers = nullptr;
    AccountController *accounts = nullptr;
    Emotes *emotes = nullptr;
    NativeMessaging *nativeMessaging = nullptr;
    Fonts *fonts = nullptr;
    Resources *resources = nullptr;
    ModerationActions *moderationActions = nullptr;
    TwitchServer *twitch2 = nullptr;

    /// Provider-specific
    struct {
        [[deprecated("use twitch2 instead")]] TwitchServer *server = nullptr;
        [[deprecated("use twitch2->pubsub instead")]] PubSub *pubsub = nullptr;
    } twitch;

    void save();

    // Special application mode that only initializes the native messaging host
    static void runNativeMessagingHost();

private:
    void addSingleton(Singleton *singleton);

    int argc_;
    char **argv_;

    std::vector<Singleton *> singletons_;
};

Application *getApp();

bool appInitialized();

}  // namespace chatterino
