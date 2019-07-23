#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/WindowManager.hpp"
#include "util/WindowsHelper.hpp"

namespace chatterino {

Settings *Settings::instance = nullptr;

Settings::Settings(const QString &settingsDirectory)
    : ABSettings(settingsDirectory)
{
    instance = this;

#ifdef USEWINSDK
    this->autorun = isRegisteredForStartup();
    this->autorun.connect(
        [](bool autorun) { setRegisteredForStartup(autorun); }, false);
#endif
}

Settings &Settings::getInstance()
{
    return *instance;
}

Settings *getSettings()
{
    return &Settings::getInstance();
}

}  // namespace chatterino
