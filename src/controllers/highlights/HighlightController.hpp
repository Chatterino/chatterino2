#pragma once

#include "controllers/highlights/HighlightPhrase.hpp"
#include "messages/Message.hpp"
#include "singletons/SettingsManager.hpp"
#include "common/SignalVector2.hpp"

namespace chatterino {
namespace controllers {
namespace highlights {

class HighlightModel;

class HighlightController
{
public:
    HighlightController();

    void initialize();

    util::UnsortedSignalVector<HighlightPhrase> phrases;

    HighlightModel *createModel(QObject *parent);

    void addHighlight(const messages::MessagePtr &msg);

private:
    bool initialized = false;

    singletons::ChatterinoSetting<std::vector<highlights::HighlightPhrase>> highlightsSetting = {
        "/highlighting/highlights"};
};

}  // namespace highlights
}  // namespace controllers
}  // namespace chatterino
