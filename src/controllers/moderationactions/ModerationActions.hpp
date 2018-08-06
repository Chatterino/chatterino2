#pragma once

#include "common/Singleton.hpp"

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"

namespace chatterino {

class Settings;
class Paths;

class ModerationActionModel;

class ModerationActions final : public Singleton
{
public:
    ModerationActions();

    virtual void initialize(Settings &settings, Paths &paths) override;

    UnsortedSignalVector<ModerationAction> items;

    ModerationActionModel *createModel(QObject *parent);

private:
    ChatterinoSetting<std::vector<ModerationAction>> setting_ = {"/moderation/actions"};
    bool initialized_ = false;
};

}  // namespace chatterino
