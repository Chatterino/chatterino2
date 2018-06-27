#include "HighlightController.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "widgets/dialogs/NotificationPopup.hpp"

namespace chatterino {

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
        this->highlightsSetting.setValue(this->phrases.getVector());
    });
}

HighlightModel *HighlightController::createModel(QObject *parent)
{
    HighlightModel *model = new HighlightModel(parent);
    model->init(&this->phrases);

    return model;
}

void HighlightController::addHighlight(const chatterino::MessagePtr &msg)
{
    //    static NotificationPopup popup;

    //    popup.updatePosition();
    //    popup.addMessage(msg);
    //    popup.show();
}

}  // namespace chatterino
