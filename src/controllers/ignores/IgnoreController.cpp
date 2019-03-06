#include "controllers/ignores/IgnoreController.hpp"

#include "Application.hpp"
#include "controllers/ignores/IgnoreModel.hpp"

#include <cassert>

namespace chatterino
{
    void IgnoreController::initialize(Settings&, Paths&)
    {
        assert(!this->initialized_);
        this->initialized_ = true;

        for (const IgnorePhrase& phrase : this->ignoresSetting_.getValue())
        {
            this->phrases.appendItem(phrase);
        }

        this->phrases.delayedItemsChanged.connect([this] {  //
            this->ignoresSetting_.setValue(this->phrases.getVector());
        });
    }

    IgnoreModel* IgnoreController::createModel(QObject* parent)
    {
        IgnoreModel* model = new IgnoreModel(parent);
        model->init(&this->phrases);

        return model;
    }

}  // namespace chatterino
