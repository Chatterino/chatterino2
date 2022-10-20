#include "providers/seventv/SeventvEventApi.hpp"

namespace chatterino {

bool SeventvEventApiSubscription::operator==(
    const SeventvEventApiSubscription &rhs) const
{
    return std::tie(this->condition, this->type) ==
           std::tie(rhs.condition, rhs.type);
}
bool SeventvEventApiSubscription::operator!=(
    const SeventvEventApiSubscription &rhs) const
{
    return !(rhs == *this);
}

}  // namespace chatterino
