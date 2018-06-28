#pragma once

#include "common/SignalVector.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "common/ChatterinoSetting.hpp"

namespace chatterino {

class ModerationActions
{
public:
    ModerationActions();

    void initialize();

    UnsortedSignalVector<ModerationAction> items;

private:
    ChatterinoSetting<std::vector<ModerationAction>> setting = {"/moderation/actions"};
    bool initialized = false;
};

}  // namespace chatterino
