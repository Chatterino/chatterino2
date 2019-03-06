#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Paths.hpp"
#include "singletons/WindowManager.hpp"

namespace chatterino
{
    Settings* Settings::instance = nullptr;

    Settings::Settings(const QString& settingsDirectory)
        : ABSettings(settingsDirectory)
    {
        instance = this;
    }

    Settings& Settings::getInstance()
    {
        return *instance;
    }

    Settings* getSettings()
    {
        return &Settings::getInstance();
    }

}  // namespace chatterino
