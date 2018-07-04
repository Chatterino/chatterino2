#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"

namespace chatterino {

class ModerationActionModel;

class ModerationActions
{
public:
    ModerationActions();

    void initialize();

    UnsortedSignalVector<ModerationAction> items;

    ModerationActionModel *createModel(QObject *parent);

private:
    ChatterinoSetting<std::vector<ModerationAction>> setting = {"/moderation/actions"};
    bool initialized = false;
};

}  // namespace chatterino
