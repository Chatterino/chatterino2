#pragma once

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"

namespace chatterino {

class Settings;
class Paths;

class ModerationActions final : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    SignalVector<ModerationAction> items;

private:
    bool initialized_ = false;
};

}  // namespace chatterino
