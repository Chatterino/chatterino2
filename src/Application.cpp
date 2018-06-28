#include "Application.hpp"

#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/moderationactions/ModerationActions.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "providers/twitch/Pubsub.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Logging.hpp"
#include "singletons/NativeMessaging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/IsBigEndian.hpp"
#include "util/PostToThread.hpp"

#include <atomic>

namespace chatterino {

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
    this->settings = getSettings();
    this->paths = Paths::getInstance();

    this->themes = new Theme;
    this->windows = new WindowManager;
    this->logging = new Logging;
    this->commands = new CommandController;
    this->highlights = new HighlightController;
    this->ignores = new IgnoreController;
    this->taggedUsers = new TaggedUsersController;
    this->accounts = new AccountController;
    this->emotes = new Emotes;
    this->fonts = new Fonts;
    this->resources = new Resources;
    this->moderationActions = new ModerationActions;

    this->twitch.server = new TwitchServer;
    this->twitch.pubsub = new PubSub;
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
    this->settings->initialize();

    this->settings->load();
    this->commands->load();
    this->logging->initialize();
    this->windows->initialize();

    this->resources->initialize();

    this->highlights->initialize();
    this->ignores->initialize();

    this->emotes->initialize();

    this->accounts->load();

    this->twitch.server->initialize();
    this->moderationActions->initialize();

    // XXX
    this->windows->updateWordTypeMask();

#ifdef Q_OS_WIN
#ifdef QT_DEBUG
#ifdef C_DEBUG_NM
    this->nativeMessaging->registerHost();
    this->nativeMessaging->openGuiMessageQueue();
#endif
#else
    this->nativeMessaging->registerHost();
    this->nativeMessaging->openGuiMessageQueue();
#endif
#endif

    this->twitch.pubsub->signals_.whisper.sent.connect([](const auto &msg) {
        Log("WHISPER SENT LOL");  //
    });

    this->twitch.pubsub->signals_.whisper.received.connect([](const auto &msg) {
        Log("WHISPER RECEIVED LOL");  //
    });

    this->twitch.pubsub->signals_.moderation.chatCleared.connect([this](const auto &action) {
        auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);
        if (chan->isEmpty()) {
            return;
        }

        QString text = QString("%1 cleared the chat").arg(action.source.name);

        auto msg = Message::createSystemMessage(text);
        postToThread([chan, msg] { chan->addMessage(msg); });
    });

    this->twitch.pubsub->signals_.moderation.modeChanged.connect([this](const auto &action) {
        auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);
        if (chan->isEmpty()) {
            return;
        }

        QString text = QString("%1 turned %2 %3 mode")  //
                           .arg(action.source.name)
                           .arg(action.state == ModeChangedAction::State::On ? "on" : "off")
                           .arg(action.getModeName());

        if (action.duration > 0) {
            text.append(" (" + QString::number(action.duration) + " seconds)");
        }

        auto msg = Message::createSystemMessage(text);
        postToThread([chan, msg] { chan->addMessage(msg); });
    });

    this->twitch.pubsub->signals_.moderation.moderationStateChanged.connect(
        [this](const auto &action) {
            auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty()) {
                return;
            }

            QString text;

            if (action.modded) {
                text = QString("%1 modded %2").arg(action.source.name, action.target.name);
            } else {
                text = QString("%1 unmodded %2").arg(action.source.name, action.target.name);
            }

            auto msg = Message::createSystemMessage(text);
            postToThread([chan, msg] { chan->addMessage(msg); });
        });

    this->twitch.pubsub->signals_.moderation.userBanned.connect([&](const auto &action) {
        auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);

        if (chan->isEmpty()) {
            return;
        }

        auto msg = Message::createTimeoutMessage(action);
        msg->flags |= Message::PubSub;

        postToThread([chan, msg] { chan->addOrReplaceTimeout(msg); });
    });

    this->twitch.pubsub->signals_.moderation.userUnbanned.connect([&](const auto &action) {
        auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);

        if (chan->isEmpty()) {
            return;
        }

        auto msg = Message::createUntimeoutMessage(action);

        postToThread([chan, msg] { chan->addMessage(msg); });
    });

    this->twitch.pubsub->start();

    auto RequestModerationActions = [=]() {
        this->twitch.pubsub->unlistenAllModerationActions();
        // TODO(pajlada): Unlisten to all authed topics instead of only moderation topics
        // this->twitch.pubsub->UnlistenAllAuthedTopics();

        this->twitch.pubsub->listenToWhispers(this->accounts->twitch.getCurrent());  //
    };

    this->accounts->twitch.currentUserChanged.connect(RequestModerationActions);

    RequestModerationActions();
}

int Application::run(QApplication &qtApp)
{
    // Start connecting to the IRC Servers (Twitch only for now)
    this->twitch.server->connect();

    // Show main window
    this->windows->getMainWindow().show();

    return qtApp.exec();
}

void Application::save()
{
    this->windows->save();

    this->commands->save();
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
