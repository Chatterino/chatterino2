#pragma once

#include "common/SignalVector2.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "messages/Message.hpp"
#include "singletons/SettingsManager.hpp"

namespace chatterino {

class HighlightModel;

class HighlightController
{
public:
    HighlightController();

    void initialize();

    UnsortedSignalVector<HighlightPhrase> phrases;

    HighlightModel *createModel(QObject *parent);

    void addHighlight(const chatterino::MessagePtr &msg);

private:
    bool initialized = false;

    chatterino::ChatterinoSetting<std::vector<HighlightPhrase>> highlightsSetting = {
        "/highlighting/highlights"};
};

}  // namespace chatterino
