#pragma once

#include "common/Singleton.hpp"

#include "common/SignalVector.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class IgnoreModel;

class IgnoreController : public Singleton
{
public:
    virtual void initialize(Application &app) override;

    UnsortedSignalVector<IgnorePhrase> phrases;

    IgnoreModel *createModel(QObject *parent);

private:
    bool initialized_ = false;

    ChatterinoSetting<std::vector<IgnorePhrase>> ignoresSetting_ = {"/ignore/phrases"};
};

}  // namespace chatterino
