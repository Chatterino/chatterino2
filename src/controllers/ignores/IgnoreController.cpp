#include "controllers/ignores/IgnoreController.hpp"

#include "Application.hpp"
#include "controllers/ignores/IgnoreModel.hpp"
#include "util/PersistSignalVector.hpp"

#include <cassert>

namespace chatterino {

void IgnoreController::initialize(Settings &, Paths &)
{
    assert(!this->initialized_);
    this->initialized_ = true;

    persist(this->phrases, "/ignore/phrases");
}

}  // namespace chatterino
