#include "highlightcontroller.hpp"

#include "application.hpp"
#include "controllers/highlights/highlightmodel.hpp"

namespace chatterino {
namespace controllers {
namespace highlights {

HighlightController::HighlightController()
{
}

void HighlightController::initialize()
{
    assert(!this->initialized);
    this->initialized = true;

    for (const HighlightPhrase &phrase : this->highlightsSetting.getValue()) {
        this->phrases.appendItem(phrase);
    }

    this->phrases.delayedItemsChanged.connect([this] {  //
        int xd = this->phrases.getVector().size();
        this->highlightsSetting.setValue(this->phrases.getVector());
    });
}

HighlightModel *HighlightController::createModel(QObject *parent)
{
    HighlightModel *model = new HighlightModel(parent);
    model->init(&this->phrases);

    return model;
}

}  // namespace highlights
}  // namespace controllers
}  // namespace chatterino
