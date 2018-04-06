#include "application.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/commandmanager.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/loggingmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"

using namespace chatterino::singletons;

namespace chatterino {

// this class is responsible for handling the workflow of Chatterino
// It will create the instances of the major classes, and connect their signals to each other

Application::Application()
{
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
