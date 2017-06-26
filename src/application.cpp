#include "application.hpp"
#include "colorscheme.hpp"
#include "logging/loggingmanager.hpp"
#include "settingsmanager.hpp"

namespace chatterino {

// this class is responsible for handling the workflow of Chatterino
// It will create the instances of the major classes, and connect their signals to each other

Application::Application()
    : windowManager(this->channelManager, this->colorScheme)
    , colorScheme(this->windowManager)
    , emoteManager(this->windowManager, this->resources)
    , resources(this->emoteManager, this->windowManager)
    , channelManager(this->windowManager, this->emoteManager, this->ircManager)
    , ircManager(this->channelManager, this->resources, this->emoteManager, this->windowManager)
    , messageFactory(this->resources, this->emoteManager, this->windowManager)
{
    // TODO(pajlada): Get rid of all singletons
    logging::init();
    SettingsManager::getInstance().load();

    // Initialize everything we need
    this->emoteManager.loadGlobalEmotes();

    // XXX
    SettingsManager::getInstance().updateWordTypeMask();

    this->windowManager.load();

    this->ircManager.onPrivateMessage.connect([=](Communi::IrcPrivateMessage *message) {
        QString channelName = message->target().mid(1);

        auto channel = this->channelManager.getChannel(channelName);

        if (channel == nullptr) {
            // The message doesn't have a channel we listen to
            return;
        }

        messages::MessageParseArgs args;

        this->messageFactory.buildMessage(message, *channel.get(), args);
    });
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
