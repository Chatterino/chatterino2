#include "providers/twitch/PubsubActions.hpp"

#include "providers/twitch/PubsubHelpers.hpp"

namespace chatterino {

PubSubAction::PubSubAction(const rapidjson::Value &data, const QString &_roomID)
    : timestamp(std::chrono::steady_clock::now())
    , roomID(_roomID)
{
    getCreatedByUser(data, this->source);
}

}  // namespace chatterino
