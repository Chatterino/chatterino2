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

using namespace chatterino::singletons;

namespace chatterino {

// this class is responsible for handling the workflow of Chatterino
// It will create the instances of the major classes, and connect their signals to each other

Application::Application()
{
    singletons::NativeMessagingManager::getInstance().registerHost();

    singletons::WindowManager::getInstance();

    singletons::LoggingManager::getInstance();

    singletons::SettingManager::getInstance().initialize();
    singletons::CommandManager::getInstance().loadCommands();

    singletons::WindowManager::getInstance().initialize();

    // Initialize everything we need
    singletons::EmoteManager::getInstance().loadGlobalEmotes();

    singletons::AccountManager::getInstance().load();

    // XXX
    singletons::SettingManager::getInstance().updateWordTypeMask();

    singletons::NativeMessagingManager::getInstance().openGuiMessageQueue();
    auto &pubsub = singletons::PubSubManager::getInstance();

    pubsub.sig.whisper.sent.connect([](const auto &msg) {
        debug::Log("WHISPER SENT LOL");  //
    });

    pubsub.sig.whisper.received.connect([](const auto &msg) {
        debug::Log("WHISPER RECEIVED LOL");  //
    });

    pubsub.sig.moderation.chatCleared.connect([&](const auto &action) {
        debug::Log("Chat cleared by {}", action.source.name);  //
    });

    pubsub.sig.moderation.modeChanged.connect([&](const auto &action) {
        debug::Log("Mode {} was turned {} by {} (duration {})", (int &)action.mode,
                   (bool &)action.state, action.source.name, action.args.duration);
    });

    pubsub.sig.moderation.moderationStateChanged.connect([&](const auto &action) {
        debug::Log("User {} was {} by {}", action.target.id, action.modded ? "modded" : "unmodded",
                   action.source.name);
    });

    pubsub.sig.moderation.userBanned.connect([&](const auto &action) {
        auto &server = providers::twitch::TwitchServer::getInstance();
        auto chan = server.getChannelOrEmptyByID(action.roomID);

        if (chan->isEmpty()) {
            return;
        }

        auto msg = messages::Message::createTimeoutMessage(action);

        util::postToThread([chan, msg] { chan->addMessage(msg); });
    });

    pubsub.sig.moderation.userUnbanned.connect([&](const auto &action) {
        auto &server = providers::twitch::TwitchServer::getInstance();
        auto chan = server.getChannelOrEmptyByID(action.roomID);

        if (chan->isEmpty()) {
            return;
        }

        auto msg = messages::Message::createUntimeoutMessage(action);

        util::postToThread([chan, msg] { chan->addMessage(msg); });
    });

    auto &accountManager = singletons::AccountManager::getInstance();

    pubsub.Start();

    auto RequestModerationActions = [&]() {
        pubsub.UnlistenAllModerationActions();
        // TODO(pajlada): Unlisten to all authed topics instead of only moderation topics
        // pubsub.UnlistenAllAuthedTopics();

        pubsub.ListenToWhispers(singletons::AccountManager::getInstance().Twitch.getCurrent());  //
    };

    accountManager.Twitch.userChanged.connect(RequestModerationActions);

    RequestModerationActions();
}

Application::~Application()
{
    this->save();
}

int Application::run(QApplication &qtApp)
{
    // Start connecting to the IRC Servers (Twitch only for now)
    providers::twitch::TwitchServer::getInstance().connect();

    // Show main window
    singletons::WindowManager::getInstance().getMainWindow().show();

    return qtApp.exec();
}

void Application::save()
{
    singletons::WindowManager::getInstance().save();

    singletons::CommandManager::getInstance().saveCommands();
}

}  // namespace chatterino
