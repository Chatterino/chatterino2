#include "application.hpp"

#include "providers/twitch/twitchserver.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/commandmanager.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/loggingmanager.hpp"
#include "singletons/nativemessagingmanager.hpp"
#include "singletons/pathmanager.hpp"
#include "singletons/pubsubmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "util/posttothread.hpp"

#include <atomic>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

using namespace chatterino::singletons;

namespace chatterino {

namespace {

bool isBigEndian()
{
    int test = 1;
    char *p = (char *)&test;

    return p[0] == 0;
}

}  // namespace

static std::atomic<bool> isAppConstructed{false};
static std::atomic<bool> isAppInitialized{false};

static Application *staticApp = nullptr;

// this class is responsible for handling the workflow of Chatterino
// It will create the instances of the major classes, and connect their signals to each other

Application::Application(int _argc, char **_argv)
    : argc(_argc)
    , argv(_argv)
{
}

void Application::construct()
{
    assert(isAppConstructed == false);
    isAppConstructed = true;

    // 1. Instantiate all classes
    this->paths = new singletons::PathManager(argc, argv);
    this->themes = new singletons::ThemeManager;
    this->windows = new singletons::WindowManager;
    this->logging = new singletons::LoggingManager;
    this->commands = new singletons::CommandManager;
    this->accounts = new singletons::AccountManager;
    this->emotes = new singletons::EmoteManager;
    this->pubsub = new singletons::PubSubManager;
    this->settings = new singletons::SettingManager;
}

void Application::instantiate(int argc, char **argv)
{
    assert(staticApp == nullptr);

    staticApp = new Application(argc, argv);
}

void Application::initialize()
{
    assert(isAppInitialized == false);
    isAppInitialized = true;

    // 2. Initialize/load classes
    this->windows->initialize();

    this->nativeMessaging->registerHost();

    this->settings->initialize();
    this->commands->loadCommands();

    // Initialize everything we need
    this->emotes->loadGlobalEmotes();

    this->accounts->load();

    // XXX
    this->settings->updateWordTypeMask();

    this->nativeMessaging->openGuiMessageQueue();

    this->pubsub->sig.whisper.sent.connect([](const auto &msg) {
        debug::Log("WHISPER SENT LOL");  //
    });

    this->pubsub->sig.whisper.received.connect([](const auto &msg) {
        debug::Log("WHISPER RECEIVED LOL");  //
    });

    this->pubsub->sig.moderation.chatCleared.connect([&](const auto &action) {
        debug::Log("Chat cleared by {}", action.source.name);  //
    });

    this->pubsub->sig.moderation.modeChanged.connect([&](const auto &action) {
        debug::Log("Mode {} was turned {} by {} (duration {})", (int &)action.mode,
                   (bool &)action.state, action.source.name, action.args.duration);
    });

    this->pubsub->sig.moderation.moderationStateChanged.connect([&](const auto &action) {
        debug::Log("User {} was {} by {}", action.target.id, action.modded ? "modded" : "unmodded",
                   action.source.name);
    });

    this->pubsub->sig.moderation.userBanned.connect([&](const auto &action) {
        auto &server = providers::twitch::TwitchServer::getInstance();
        auto chan = server.getChannelOrEmptyByID(action.roomID);

        if (chan->isEmpty()) {
            return;
        }

        auto msg = messages::Message::createTimeoutMessage(action);

        util::postToThread([chan, msg] { chan->addMessage(msg); });
    });

    this->pubsub->sig.moderation.userUnbanned.connect([&](const auto &action) {
        auto &server = providers::twitch::TwitchServer::getInstance();
        auto chan = server.getChannelOrEmptyByID(action.roomID);

        if (chan->isEmpty()) {
            return;
        }

        auto msg = messages::Message::createUntimeoutMessage(action);

        util::postToThread([chan, msg] { chan->addMessage(msg); });
    });

    this->pubsub->Start();

    auto RequestModerationActions = [=]() {
        this->pubsub->UnlistenAllModerationActions();
        // TODO(pajlada): Unlisten to all authed topics instead of only moderation topics
        // this->pubsub->UnlistenAllAuthedTopics();

        this->pubsub->ListenToWhispers(this->accounts->Twitch.getCurrent());  //
    };

    this->accounts->Twitch.userChanged.connect(RequestModerationActions);

    RequestModerationActions();
}

int Application::run(QApplication &qtApp)
{
    // Start connecting to the IRC Servers (Twitch only for now)
    providers::twitch::TwitchServer::getInstance().connect();

    // Show main window
    this->windows->getMainWindow().show();

    return qtApp.exec();
}

void Application::save()
{
    this->windows->save();

    this->commands->saveCommands();
}

void Application::runNativeMessagingHost()
{
    auto app = getApp();

    app->nativeMessaging = new singletons::NativeMessagingManager;

#ifdef Q_OS_WIN
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

#if 0
    bool bigEndian = isBigEndian();
#endif

    while (true) {
        char size_c[4];
        std::cin.read(size_c, 4);

        if (std::cin.eof()) {
            break;
        }

        uint32_t size = *reinterpret_cast<uint32_t *>(size_c);
#if 0
        // To avoid breaking strict-aliasing rules and potentially inducing undefined behaviour, the following code can be run instead
        uint32_t size = 0;
        if (bigEndian) {
            size = size_c[3] | static_cast<uint32_t>(size_c[2]) << 8 |
                   static_cast<uint32_t>(size_c[1]) << 16 | static_cast<uint32_t>(size_c[0]) << 24;
        } else {
            size = size_c[0] | static_cast<uint32_t>(size_c[1]) << 8 |
                   static_cast<uint32_t>(size_c[2]) << 16 | static_cast<uint32_t>(size_c[3]) << 24;
        }
#endif

        char *b = (char *)malloc(size + 1);
        std::cin.read(b, size);
        *(b + size) = '\0';

        app->nativeMessaging->sendToGuiProcess(QByteArray(b, size));

        free(b);
    }
}

Application *getApp()
{
    assert(staticApp != nullptr);

    return staticApp;
}

bool appInitialized()
{
    return isAppInitialized;
}

}  // namespace chatterino
