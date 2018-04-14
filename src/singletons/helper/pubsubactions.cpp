#include "singletons/helper/pubsubactions.hpp"

#include "singletons/helper/pubsubhelpers.hpp"

namespace chatterino {
namespace singletons {

PubSubAction::PubSubAction(const rapidjson::Value &data)
    : timestamp(std::chrono::steady_clock::now())
{
    getCreatedByUser(data, this->source);
}

}  // namespace singletons
}  // namespace chatterino
