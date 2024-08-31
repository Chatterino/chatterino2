#pragma once

#include <boost/functional/hash.hpp>
#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QString>

#include <variant>

namespace chatterino {

struct BttvLiveUpdateSubscriptionChannel {
    QString twitchID;

    QJsonObject encode(bool isSubscribe) const;
    bool operator==(const BttvLiveUpdateSubscriptionChannel &rhs) const;
    bool operator!=(const BttvLiveUpdateSubscriptionChannel &rhs) const;
    friend QDebug &operator<<(QDebug &dbg,
                              const BttvLiveUpdateSubscriptionChannel &data);
};

struct BttvLiveUpdateBroadcastMe {
    QString twitchID;
    QString userName;

    QJsonObject encode(bool isSubscribe) const;
    bool operator==(const BttvLiveUpdateBroadcastMe &rhs) const;
    bool operator!=(const BttvLiveUpdateBroadcastMe &rhs) const;
    friend QDebug &operator<<(QDebug &dbg,
                              const BttvLiveUpdateBroadcastMe &data);
};

using BttvLiveUpdateSubscriptionData =
    std::variant<BttvLiveUpdateSubscriptionChannel, BttvLiveUpdateBroadcastMe>;

struct BttvLiveUpdateSubscription {
    BttvLiveUpdateSubscriptionData data;

    QByteArray encodeSubscribe() const;
    QByteArray encodeUnsubscribe() const;

    bool operator==(const BttvLiveUpdateSubscription &rhs) const
    {
        return this->data == rhs.data;
    }
    bool operator!=(const BttvLiveUpdateSubscription &rhs) const
    {
        return !(*this == rhs);
    }

    friend QDebug &operator<<(QDebug &dbg,
                              const BttvLiveUpdateSubscription &subscription);
};

}  // namespace chatterino

namespace std {

template <>
struct hash<chatterino::BttvLiveUpdateSubscriptionChannel> {
    size_t operator()(
        const chatterino::BttvLiveUpdateSubscriptionChannel &data) const
    {
        return qHash(data.twitchID);
    }
};

template <>
struct hash<chatterino::BttvLiveUpdateBroadcastMe> {
    size_t operator()(const chatterino::BttvLiveUpdateBroadcastMe &data) const
    {
        size_t seed = 0;
        boost::hash_combine(seed, qHash(data.twitchID));
        boost::hash_combine(seed, qHash(data.userName));
        return seed;
    }
};

template <>
struct hash<chatterino::BttvLiveUpdateSubscription> {
    size_t operator()(const chatterino::BttvLiveUpdateSubscription &sub) const
    {
        return std::hash<chatterino::BttvLiveUpdateSubscriptionData>{}(
            sub.data);
    }
};

}  // namespace std
