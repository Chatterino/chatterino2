#include "singletons/helper/pubsubactions.hpp"

#include "singletons/helper/pubsubhelpers.hpp"

namespace chatterino {
namespace singletons {

PubSubAction::PubSubAction(const rapidjson::Value &data, const QString &_roomID)
    : timestamp(std::chrono::steady_clock::now())
    , roomID(_roomID)
{
    getCreatedByUser(data, this->source);
}

}  // namespace singletons
}  // namespace chatterino
