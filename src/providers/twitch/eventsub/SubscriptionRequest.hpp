#pragma once

#include <boost/functional/hash.hpp>
#include <QHash>
#include <QString>

#include <utility>
#include <vector>

namespace chatterino {

struct SubscriptionRequest {
    /// e.g. "channel.ban"
    /// can be made into an enum later
    QString subscriptionType;

    // e.g. "1"
    // maybe this should be part of the enum later
    QString subscriptionVersion;

    /// Optional list of conditions for the subscription
    std::vector<std::pair<QString, QString>> conditions;
};

bool operator==(const SubscriptionRequest &lhs, const SubscriptionRequest &rhs);
bool operator!=(const SubscriptionRequest &lhs, const SubscriptionRequest &rhs);

}  // namespace chatterino

namespace std {

template <>
struct hash<chatterino::SubscriptionRequest> {
    size_t operator()(const chatterino::SubscriptionRequest &v) const
    {
        size_t seed = 0;

        boost::hash_combine(seed, qHash(v.subscriptionType));
        boost::hash_combine(seed, qHash(v.subscriptionVersion));

        for (const auto &[conditionKey, conditionValue] : v.conditions)
        {
            boost::hash_combine(seed, qHash(conditionKey));
            boost::hash_combine(seed, qHash(conditionValue));
        }

        return seed;
    }
};

}  // namespace std
