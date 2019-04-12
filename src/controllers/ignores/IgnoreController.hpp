#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"

namespace chatterino {

class Settings;
class Paths;

class IgnoreModel;

enum ShowIgnoredUsersMessages { Never, IfModerator, IfBroadcaster };

class IgnoreController final : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    UnsortedSignalVector<IgnorePhrase> phrases;

    IgnoreModel *createModel(QObject *parent);

private:
    bool initialized_ = false;

    ChatterinoSetting<std::vector<IgnorePhrase>> ignoresSetting_ = {
        "/ignore/phrases"};
};

}  // namespace chatterino
