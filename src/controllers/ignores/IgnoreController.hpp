#pragma once

#include "common/SignalVector2.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class IgnoreModel;

class IgnoreController
{
public:
    void initialize();

    UnsortedSignalVector<IgnorePhrase> phrases;

    IgnoreModel *createModel(QObject *parent);

private:
    bool initialized = false;

    ChatterinoSetting<std::vector<IgnorePhrase>> ignoresSetting = {"/ignore/phrases"};
};

}  // namespace chatterino
