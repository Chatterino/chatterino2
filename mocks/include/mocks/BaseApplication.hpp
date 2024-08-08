#pragma once

#include "mocks/DisabledStreamerMode.hpp"
#include "mocks/EmptyApplication.hpp"
#include "providers/bttv/BttvLiveUpdates.hpp"
#include "singletons/Settings.hpp"

#include <QString>

namespace chatterino::mock {

/**
 * BaseApplication intends to be a mock application with a few more sane defaults, but with less configurability
 */
class BaseApplication : public EmptyApplication
{
public:
    BaseApplication()
        : settings(this->settingsDir.filePath("settings.json"))
    {
    }

    explicit BaseApplication(const QString &settingsData)
        : EmptyApplication(settingsData)
        , settings(this->settingsDir.filePath("settings.json"))
    {
    }

    IStreamerMode *getStreamerMode() override
    {
        return &this->streamerMode;
    }

    BttvLiveUpdates *getBttvLiveUpdates() override
    {
        return nullptr;
    }

    SeventvEventAPI *getSeventvEventAPI() override
    {
        return nullptr;
    }

    Settings settings;
    DisabledStreamerMode streamerMode;
};

}  // namespace chatterino::mock
