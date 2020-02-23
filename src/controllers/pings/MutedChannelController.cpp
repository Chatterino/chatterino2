#include "controllers/pings/MutedChannelController.hpp"
#include "controllers/pings/MutedChannelModel.hpp"
#include "util/PersistSignalVector.hpp"

namespace chatterino {

void MutedChannelController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
}

}  // namespace chatterino
