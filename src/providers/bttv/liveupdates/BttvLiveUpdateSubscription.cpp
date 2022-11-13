#include "providers/bttv/liveupdates/BttvLiveUpdateSubscription.hpp"

#include <QJsonDocument>
#include <QJsonObject>

namespace {
using namespace chatterino;

const char *typeToString(BttvLiveUpdateSubscriptionType type, bool isSubscribe)
{
    switch (type)
    {
        case BttvLiveUpdateSubscriptionType::Channel:
            return isSubscribe ? "join_channel" : "part_channel";
        default:
            return "";
    }
}

QJsonObject createDataJson(const QString &twitchId)
{
    QJsonObject data;
    data["name"] = QString("twitch:%1").arg(twitchId);
    return data;
}

}  // namespace

namespace chatterino {

bool BttvLiveUpdateSubscription::operator==(
    const BttvLiveUpdateSubscription &rhs) const
{
    return std::tie(this->twitchID, this->type) ==
           std::tie(rhs.twitchID, rhs.type);
}

bool BttvLiveUpdateSubscription::operator!=(
    const BttvLiveUpdateSubscription &rhs) const
{
    return !(rhs == *this);
}

QByteArray BttvLiveUpdateSubscription::encodeSubscribe() const
{
    const auto *typeName = typeToString(this->type, true);
    QJsonObject root;
    root["name"] = typeName;
    root["data"] = createDataJson(this->twitchID);
    return QJsonDocument(root).toJson();
}

QByteArray BttvLiveUpdateSubscription::encodeUnsubscribe() const
{
    const auto *typeName = typeToString(this->type, false);
    QJsonObject root;
    root["name"] = typeName;
    root["data"] = createDataJson(this->twitchID);
    return QJsonDocument(root).toJson();
}

QDebug &operator<<(QDebug &dbg, const BttvLiveUpdateSubscription &subscription)
{
    dbg << "BttvLiveUpdateSubscription{ twitchID:" << subscription.twitchID
        << "type:" << (int)subscription.type << '}';
    return dbg;
}

}  // namespace chatterino
