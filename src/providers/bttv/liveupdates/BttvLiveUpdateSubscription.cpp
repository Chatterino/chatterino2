#include "providers/bttv/liveupdates/BttvLiveUpdateSubscription.hpp"

#include <QDebug>
#include <QJsonDocument>

namespace chatterino {

QByteArray BttvLiveUpdateSubscription::encodeSubscribe() const
{
    return QJsonDocument(std::visit(
                             [](const auto &d) {
                                 return d.encode(true);
                             },
                             this->data))
        .toJson();
}

QByteArray BttvLiveUpdateSubscription::encodeUnsubscribe() const
{
    return QJsonDocument(std::visit(
                             [](const auto &d) {
                                 return d.encode(false);
                             },
                             this->data))
        .toJson();
}

QDebug &operator<<(QDebug &dbg, const BttvLiveUpdateSubscription &subscription)
{
    std::visit(
        [&](const auto &data) {
            dbg << data;
        },
        subscription.data);
    return dbg;
}

QJsonObject BttvLiveUpdateSubscriptionChannel::encode(bool isSubscribe) const
{
    QJsonObject root;
    if (isSubscribe)
    {
        root["name"] = "join_channel";
    }
    else
    {
        root["name"] = "part_channel";
    }

    QJsonObject data;
    data["name"] = QString("twitch:%1").arg(this->twitchID);

    root["data"] = data;
    return root;
}

bool BttvLiveUpdateSubscriptionChannel::operator==(
    const BttvLiveUpdateSubscriptionChannel &rhs) const
{
    return this->twitchID == rhs.twitchID;
}

bool BttvLiveUpdateSubscriptionChannel::operator!=(
    const BttvLiveUpdateSubscriptionChannel &rhs) const
{
    return !(*this == rhs);
}

QDebug &operator<<(QDebug &dbg, const BttvLiveUpdateSubscriptionChannel &data)
{
    dbg << "BttvLiveUpdateSubscriptionChannel{ twitchID:" << data.twitchID
        << '}';
    return dbg;
}

QJsonObject BttvLiveUpdateBroadcastMe::encode(bool /*isSubscribe*/) const
{
    QJsonObject root;
    root["name"] = "broadcast_me";

    QJsonObject data;
    data["name"] = this->userName;
    data["channel"] = QString("twitch:%1").arg(this->twitchID);

    root["data"] = data;
    return root;
}

bool BttvLiveUpdateBroadcastMe::operator==(
    const BttvLiveUpdateBroadcastMe &rhs) const
{
    return this->twitchID == rhs.twitchID && this->userName == rhs.userName;
}

bool BttvLiveUpdateBroadcastMe::operator!=(
    const BttvLiveUpdateBroadcastMe &rhs) const
{
    return !(*this == rhs);
}

QDebug &operator<<(QDebug &dbg, const BttvLiveUpdateBroadcastMe &data)
{
    dbg << "BttvLiveUpdateBroadcastMe{ twitchID:" << data.twitchID
        << "userName:" << data.userName << '}';
    return dbg;
}

}  // namespace chatterino
