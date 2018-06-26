#pragma once

#include "controllers/ignores/IgnorePhrase.hpp"
#include "singletons/SettingsManager.hpp"
#include "util/SignalVector2.hpp"

namespace chatterino {
namespace controllers {
namespace ignores {

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

}  // namespace ignores
}  // namespace controllers
}  // namespace chatterino
