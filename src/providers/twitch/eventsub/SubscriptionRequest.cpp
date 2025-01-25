#include "providers/twitch/eventsub/SubscriptionRequest.hpp"

namespace chatterino {

bool operator==(const SubscriptionRequest &lhs, const SubscriptionRequest &rhs)
{
    return std::tie(lhs.subscriptionType, lhs.subscriptionVersion,
                    lhs.conditions) == std::tie(rhs.subscriptionType,
                                                rhs.subscriptionVersion,
                                                rhs.conditions);
}

bool operator!=(const SubscriptionRequest &lhs, const SubscriptionRequest &rhs)
{
    return !(lhs == rhs);
}

}  // namespace chatterino
