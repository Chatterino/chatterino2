#include "application.hpp"
#include "logging/loggingmanager.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"

namespace chatterino {

// this class is responsible for handling the workflow of Chatterino
// It will create the instances of the major classes, and connect their signals to each other

Application::Application()
{
    logging::init();
    SettingsManager::getInstance().load();

    WindowManager::getInstance().initMainWindow();

    // Initialize everything we need
    EmoteManager::getInstance().loadGlobalEmotes();

    AccountManager::getInstance().load();

    // XXX
    SettingsManager::getInstance().updateWordTypeMask();
}

Application::~Application()
{
    this->save();

    chatterino::SettingsManager::getInstance().save();
}

int Application::run(QApplication &qtApp)
{
    // Start connecting to the IRC Servers (Twitch only for now)
    IrcManager::getInstance().connect();

    // Show main window
    WindowManager::getInstance().getMainWindow().show();

    return qtApp.exec();
}

void Application::save()
{
    WindowManager::getInstance().save();
}

}  // namespace chatterino
