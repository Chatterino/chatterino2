#include "providers/twitch/eventsub/SubscriptionRequest.hpp"

#include <QDebug>

namespace chatterino::eventsub {

QDebug &operator<<(QDebug &dbg, const SubscriptionRequest &v)
{
    dbg << "eventsub::SubscriptionRequest{ type:" << v.subscriptionType
        << "version:" << v.subscriptionVersion;
    if (!v.conditions.empty())
    {
        dbg << "conditions:[";
        for (const auto &[conditionKey, conditionValue] : v.conditions)
        {
            dbg << conditionKey << "=" << conditionValue << ',';
        }
        dbg << ']';
    }
    dbg << '}';
    return dbg;
}

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

}  // namespace chatterino::eventsub
