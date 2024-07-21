#pragma once

#include "mocks/DisabledStreamerMode.hpp"
#include "mocks/EmptyApplication.hpp"
#include "singletons/Settings.hpp"

namespace chatterino::mock {

/**
 * BaseApplication intends to be a mock application with a few more sane defaults, but with less configurability
 */
class BaseApplication : public EmptyApplication
{
public:
    // TODO: implement ctor where you can insert a settings body that will be loaded
    BaseApplication()
        : settings(this->settingsDir.filePath("settings.json"))
    {
    }

    IStreamerMode *getStreamerMode() override
    {
        return &this->streamerMode;
    }

    Settings settings;
    DisabledStreamerMode streamerMode;
};

}  // namespace chatterino::mock
