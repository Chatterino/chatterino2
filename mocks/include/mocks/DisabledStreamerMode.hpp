#pragma once

#include "singletons/StreamerMode.hpp"

class DisabledStreamerMode : public chatterino::IStreamerMode
{
public:
    bool isEnabled() const override
    {
        return false;
    }

    void start() override
    {
    }
};
