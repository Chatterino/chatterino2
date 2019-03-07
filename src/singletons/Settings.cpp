#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "singletons/Paths.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Log.hpp"

#include <algorithm>

namespace chatterino
{
    Settings* Settings::instance = nullptr;

    Settings::Settings(const QString& settingsDirectory)
        : BaseSettings(settingsDirectory)
    {
        Settings::instance = this;
    }

    Settings& Settings::getInstance()
    {
        assert(Settings::instance);

        return *Settings::instance;
    }

    Settings* getSettings()
    {
        return &Settings::getInstance();
    }

    float Settings::getClampedUiScale() const
    {
        return std::clamp<float>(this->uiScale.getValue(), 0.1f, 10);
    }

    void Settings::setClampedUiScale(float value)
    {
        this->uiScale.setValue(std::clamp<float>(value, 0.1f, 10));
    }
}  // namespace chatterino
