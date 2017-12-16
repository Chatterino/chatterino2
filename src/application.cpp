#include "application.hpp"
#include "accountmanager.hpp"
#include "colorscheme.hpp"
#include "logging/loggingmanager.hpp"
#include "settingsmanager.hpp"

namespace chatterino {

// this class is responsible for handling the workflow of Chatterino
// It will create the instances of the major classes, and connect their signals to each other

Application::Application()
    : completionManager(this->emoteManager)
    , windowManager(this->channelManager, this->colorScheme, this->completionManager)
    , colorScheme(this->windowManager)
    , emoteManager(this->windowManager)
    , resources(this->emoteManager, this->windowManager)
    , channelManager(this->windowManager, this->emoteManager, this->ircManager)
    , ircManager(this->channelManager, this->resources, this->emoteManager, this->windowManager)
{
    logging::init();
    SettingsManager::getInstance().load();

    this->windowManager.initMainWindow();

    // Initialize everything we need
    this->emoteManager.loadGlobalEmotes();

    AccountManager::getInstance().load();

    // XXX
    SettingsManager::getInstance().updateWordTypeMask();

    this->windowManager.load();
}

Application::~Application()
{
    this->windowManager.save();

    chatterino::SettingsManager::getInstance().save();
}

int Application::run(QApplication &qtApp)
{
    // Start connecting to the IRC Servers (Twitch only for now)
    this->ircManager.connect();

    // Show main window
    this->windowManager.getMainWindow().show();

    return qtApp.exec();
}

}  // namespace chatterino
