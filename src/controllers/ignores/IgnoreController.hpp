#pragma once

#include "controllers/ignores/IgnorePhrase.hpp"
#include "singletons/SettingsManager.hpp"
#include "common/SignalVector2.hpp"

namespace chatterino {

class IgnoreModel;

class IgnoreController
{
public:
    void initialize();

    util::UnsortedSignalVector<IgnorePhrase> phrases;

    IgnoreModel *createModel(QObject *parent);

private:
    bool initialized = false;

    singletons::ChatterinoSetting<std::vector<ignores::IgnorePhrase>> ignoresSetting = {
        "/ignore/phrases"};
};

}  // namespace chatterino
