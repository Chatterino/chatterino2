#include "application.hpp"

#include "controllers/accounts/accountcontroller.hpp"
#include "controllers/commands/commandcontroller.hpp"
#include "controllers/highlights/highlightcontroller.hpp"
#include "controllers/ignores/ignorecontroller.hpp"
#include "controllers/taggedusers/taggeduserscontroller.hpp"
#include "providers/twitch/pubsub.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/fontmanager.hpp"
#include "singletons/loggingmanager.hpp"
#include "singletons/nativemessagingmanager.hpp"
#include "singletons/pathmanager.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "util/posttothread.hpp"

#include <atomic>

using namespace chatterino::singletons;

namespace chatterino {

namespace {

bool isBigEndian()
{
    int test = 1;
    char *p = reinterpret_cast<char *>(&test);

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
    this->settings = new singletons::SettingManager;
    this->paths = singletons::PathManager::getInstance();
    this->themes = new singletons::ThemeManager;
    this->windows = new singletons::WindowManager;
    this->logging = new singletons::LoggingManager;
    this->commands = new controllers::commands::CommandController;
    this->highlights = new controllers::highlights::HighlightController;
    this->ignores = new controllers::ignores::IgnoreController;
    this->taggedUsers = new controllers::taggedusers::TaggedUsersController;
    this->accounts = new controllers::accounts::AccountController;
    this->emotes = new singletons::EmoteManager;
    this->fonts = new singletons::FontManager;
    this->resources = new singletons::ResourceManager;

    this->twitch.server = new providers::twitch::TwitchServer;
    this->twitch.pubsub = new providers::twitch::PubSub;
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

#ifdef Q_OS_WIN
    this->nativeMessaging->registerHost();
#endif

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

    // XXX
    this->settings->updateWordTypeMask();

    this->nativeMessaging->openGuiMessageQueue();

    this->twitch.pubsub->sig.whisper.sent.connect([](const auto &msg) {
        debug::Log("WHISPER SENT LOL");  //
    });

    this->twitch.pubsub->sig.whisper.received.connect([](const auto &msg) {
        debug::Log("WHISPER RECEIVED LOL");  //
    });

    this->twitch.pubsub->sig.moderation.chatCleared.connect([this](const auto &action) {
        auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);
        if (chan->isEmpty()) {
            return;
        }

        QString text = QString("%1 cleared the chat").arg(action.source.name);

        auto msg = messages::Message::createSystemMessage(text);
        util::postToThread([chan, msg] { chan->addMessage(msg); });
    });

    this->twitch.pubsub->sig.moderation.modeChanged.connect([this](const auto &action) {
        auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);
        if (chan->isEmpty()) {
            return;
        }

        QString text =
            QString("%1 turned %2 %3 mode")  //
                .arg(action.source.name)
                .arg(action.state == providers::twitch::ModeChangedAction::State::On ? "on" : "off")
                .arg(action.getModeName());

        if (action.duration > 0) {
            text.append(" (" + QString::number(action.duration) + " seconds)");
        }

        auto msg = messages::Message::createSystemMessage(text);
        util::postToThread([chan, msg] { chan->addMessage(msg); });
    });

    this->twitch.pubsub->sig.moderation.moderationStateChanged.connect([this](const auto &action) {
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

        auto msg = messages::Message::createSystemMessage(text);
        util::postToThread([chan, msg] { chan->addMessage(msg); });
    });

    this->twitch.pubsub->sig.moderation.userBanned.connect([&](const auto &action) {
        auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);

        if (chan->isEmpty()) {
            return;
        }

        auto msg = messages::Message::createTimeoutMessage(action);
        msg->flags |= messages::Message::PubSub;

        util::postToThread([chan, msg] { chan->addOrReplaceTimeout(msg); });
    });

    this->twitch.pubsub->sig.moderation.userUnbanned.connect([&](const auto &action) {
        auto chan = this->twitch.server->getChannelOrEmptyByID(action.roomID);

        if (chan->isEmpty()) {
            return;
        }

        auto msg = messages::Message::createUntimeoutMessage(action);

        util::postToThread([chan, msg] { chan->addMessage(msg); });
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
