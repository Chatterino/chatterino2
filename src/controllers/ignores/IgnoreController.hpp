#pragma once

#include "common/SignalVector.hpp"
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
    bool initialized_ = false;

    ChatterinoSetting<std::vector<IgnorePhrase>> ignoresSetting_ = {"/ignore/phrases"};
};

}  // namespace chatterino
