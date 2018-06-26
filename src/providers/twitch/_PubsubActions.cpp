#include "providers/twitch/pubsubactions.hpp"

#include "providers/twitch/pubsubhelpers.hpp"

namespace chatterino {
namespace providers {
namespace twitch {

PubSubAction::PubSubAction(const rapidjson::Value &data, const QString &_roomID)
    : timestamp(std::chrono::steady_clock::now())
    , roomID(_roomID)
{
    getCreatedByUser(data, this->source);
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
