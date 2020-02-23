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

    this->setting_ =
        std::make_unique<ChatterinoSetting<std::vector<ModerationAction>>>(
            "/moderation/actions");

    for (auto &val : this->setting_->getValue())
    {
        this->items.insert(val);
    }

    this->items.delayedItemsChanged.connect([this] {  //
        this->setting_->setValue(this->items.raw());
    });
}

ModerationActionModel *ModerationActions::createModel(QObject *parent)
{
    ModerationActionModel *model = new ModerationActionModel(parent);
    model->initialize(&this->items);

    return model;
}

}  // namespace chatterino
