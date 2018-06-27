#include "controllers/ignores/IgnoreController.hpp"

#include "Application.hpp"
#include "controllers/ignores/IgnoreModel.hpp"

#include <cassert>

namespace chatterino {

void IgnoreController::initialize()
{
    assert(!this->initialized);
    this->initialized = true;

    for (const IgnorePhrase &phrase : this->ignoresSetting.getValue()) {
        this->phrases.appendItem(phrase);
    }

    this->phrases.delayedItemsChanged.connect([this] {  //
        this->ignoresSetting.setValue(this->phrases.getVector());
    });
}

IgnoreModel *IgnoreController::createModel(QObject *parent)
{
    IgnoreModel *model = new IgnoreModel(parent);
    model->init(&this->phrases);

    return model;
}

}  // namespace chatterino
