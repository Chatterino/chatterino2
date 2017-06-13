#include "application.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"

namespace chatterino {

Application::Application()
    : windowManager(this->channelManager)
    , emoteManager(this->windowManager, this->resources)
    , resources(this->emoteManager, this->windowManager)
    , channelManager(this->windowManager, this->emoteManager, this->ircManager)
    , ircManager(this->channelManager, this->resources, this->emoteManager, this->windowManager)
{
    // TODO(pajlada): Get rid of all singletons
    ColorScheme::getInstance().init(this->windowManager);

    // Initialize everything we need
    this->emoteManager.loadGlobalEmotes();

    // XXX
    SettingsManager::getInstance().updateWordTypeMask();

    this->windowManager.load();
}

Application::~Application()
{
    this->windowManager.save();
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
