#pragma once

#include "controllers/ignores/ignorephrase.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/signalvector2.hpp"

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
