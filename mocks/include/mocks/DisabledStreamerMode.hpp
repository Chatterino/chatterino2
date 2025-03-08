#pragma once

#include "singletons/StreamerMode.hpp"

class DisabledStreamerMode : public chatterino::IStreamerMode
{
public:
    bool isEnabled() const override
    {
        return false;
    }

    bool shouldHideModActions() const override
    {
        return false;
    }

    bool shouldHideSuspiciousUsers() const override
    {
        return false;
    }

    void start() override
    {
    }
};
