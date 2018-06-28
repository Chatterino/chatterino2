#include "ModerationActions.hpp"

#include "Application.hpp"
#include "singletons/SettingsManager.hpp"

#include <QRegularExpression>

namespace chatterino {

ModerationActions::ModerationActions()
{
}

void ModerationActions::initialize()
{
    assert(!this->initialized);
    this->initialized = true;

    for (auto &val : this->setting.getValue()) {
        this->items.insertItem(val);
    }

    this->items.delayedItemsChanged.connect([this] {  //
        this->setting.setValue(this->items.getVector());
    });
}

}  // namespace chatterino
