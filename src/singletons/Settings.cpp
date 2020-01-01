#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/WindowManager.hpp"
#include "util/WindowsHelper.hpp"

namespace chatterino {

Settings *Settings::instance_ = nullptr;

Settings::Settings(const QString &settingsDirectory)
    : ABSettings(settingsDirectory)
{
    instance_ = this;

#ifdef USEWINSDK
    this->autorun = isRegisteredForStartup();
    this->autorun.connect(
        [](bool autorun) { setRegisteredForStartup(autorun); }, false);
#endif
}

Settings &Settings::instance()
{
    return *instance_;
}

Settings *getSettings()
{
    return &Settings::instance();
}

}  // namespace chatterino
