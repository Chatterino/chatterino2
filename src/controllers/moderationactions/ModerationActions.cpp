#include "ModerationActions.hpp"

#include "Application.hpp"
#include "controllers/moderationactions/ModerationActionModel.hpp"
#include "singletons/Settings.hpp"

#include <QRegularExpression>

namespace chatterino {

ModerationActions::ModerationActions()
{
}

void ModerationActions::initialize(Settings &settings, Paths &paths)
{
    assert(!this->initialized_);
    this->initialized_ = true;

    for (auto &val : this->setting_.getValue()) {
        this->items.insertItem(val);
    }

    this->items.delayedItemsChanged.connect([this] {  //
        this->setting_.setValue(this->items.getVector());
    });
}

ModerationActionModel *ModerationActions::createModel(QObject *parent)
{
    ModerationActionModel *model = new ModerationActionModel(parent);
    model->init(&this->items);

    return model;
}

}  // namespace chatterino
