// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <boost/functional/hash.hpp>
#include <QHash>
#include <QString>

#include <utility>
#include <vector>

namespace chatterino::eventsub {

struct SubscriptionRequest {
    /// e.g. "channel.ban"
    /// can be made into an enum later
    QString subscriptionType;

    // e.g. "1"
    // maybe this should be part of the enum later
    QString subscriptionVersion;

    /// The ID of the Twitch User that wants to create this subscription
    ///
    /// Since a single websocket connection cannot share connections between users,
    /// we need to key them by the user id even if it's not used in the condition.
    QString ownerTwitchUserID;

    /// Optional list of conditions for the subscription
    std::vector<std::pair<QString, QString>> conditions;

    friend QDebug operator<<(QDebug dbg, const SubscriptionRequest &v);
};

bool operator==(const SubscriptionRequest &lhs, const SubscriptionRequest &rhs);
bool operator!=(const SubscriptionRequest &lhs, const SubscriptionRequest &rhs);

}  // namespace chatterino::eventsub

namespace std {

template <>
struct hash<chatterino::eventsub::SubscriptionRequest> {
    size_t operator()(const chatterino::eventsub::SubscriptionRequest &v) const
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
