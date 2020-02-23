#include "ModerationActions.hpp"

#include "Application.hpp"
#include "controllers/moderationactions/ModerationActionModel.hpp"
#include "util/PersistSignalVector.hpp"

#include <QRegularExpression>

namespace chatterino {

void ModerationActions::initialize(Settings &settings, Paths &paths)
{
    assert(!this->initialized_);
    this->initialized_ = true;

    persist(this->items, "/moderation/actions");
}

}  // namespace chatterino
