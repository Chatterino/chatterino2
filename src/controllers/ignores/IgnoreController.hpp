#pragma once

#include "common/Singleton.hpp"

#include "common/SignalVector.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class Settings;
class Paths;

class IgnoreModel;

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
