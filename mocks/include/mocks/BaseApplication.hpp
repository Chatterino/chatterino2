#pragma once

#include "common/Args.hpp"
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
        : settings(this->args, this->settingsDir.path())
    {
    }

    explicit BaseApplication(const QString &settingsData)
        : EmptyApplication(settingsData)
        , settings(this->args, this->settingsDir.path())
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

    Args args;
    Settings settings;
    DisabledStreamerMode streamerMode;
};

}  // namespace chatterino::mock
