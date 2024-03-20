#include "providers/seventv/eventapi/Subscription.hpp"

#include "util/QMagicEnum.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include <tuple>
#include <utility>

namespace {

using namespace chatterino;
using namespace chatterino::seventv::eventapi;

QString typeToString(SubscriptionType type)
{
    return qmagicenum::enumNameString(type);
}

QJsonObject createDataJson(const QString &typeName, const Condition &condition)
{
    QJsonObject data;
    data["type"] = typeName;
    data["condition"] = std::visit(
        [](const auto &c) {
            return c.encode();
        },
        condition);
    return data;
}

}  // namespace

namespace chatterino::seventv::eventapi {

bool Subscription::operator==(const Subscription &rhs) const
{
    return std::tie(this->condition, this->type) ==
           std::tie(rhs.condition, rhs.type);
}

bool Subscription::operator!=(const Subscription &rhs) const
{
    return !(rhs == *this);
}

QByteArray Subscription::encodeSubscribe() const
{
    auto typeName = typeToString(this->type);
    QJsonObject root;
    root["op"] = (int)Opcode::Subscribe;
    root["d"] = createDataJson(typeName, this->condition);
    return QJsonDocument(root).toJson();
}

QByteArray Subscription::encodeUnsubscribe() const
{
    auto typeName = typeToString(this->type);
    QJsonObject root;
    root["op"] = (int)Opcode::Unsubscribe;
    root["d"] = createDataJson(typeName, this->condition);
    return QJsonDocument(root).toJson();
}

QDebug &operator<<(QDebug &dbg, const Subscription &subscription)
{
    std::visit(
        [&](const auto &cond) {
            dbg << "Subscription{ condition:" << cond
                << "type:" << qmagicenum::enumName(subscription.type) << '}';
        },
        subscription.condition);
    return dbg;
}

ObjectIDCondition::ObjectIDCondition(QString objectID)
    : objectID(std::move(objectID))
{
}

QJsonObject ObjectIDCondition::encode() const
{
    QJsonObject obj;
    obj["object_id"] = this->objectID;

    return obj;
}

bool ObjectIDCondition::operator==(const ObjectIDCondition &rhs) const
{
    return this->objectID == rhs.objectID;
}

bool ObjectIDCondition::operator!=(const ObjectIDCondition &rhs) const
{
    return !(*this == rhs);
}

QDebug &operator<<(QDebug &dbg, const ObjectIDCondition &condition)
{
    dbg << "{ objectID:" << condition.objectID << "}";
    return dbg;
}

ChannelCondition::ChannelCondition(QString twitchID)
    : twitchID(std::move(twitchID))
{
}

QJsonObject ChannelCondition::encode() const
{
    QJsonObject obj;
    obj["ctx"] = "channel";
    obj["platform"] = "TWITCH";
    obj["id"] = this->twitchID;
    return obj;
}

QDebug &operator<<(QDebug &dbg, const ChannelCondition &condition)
{
    dbg << "{ twitchID:" << condition.twitchID << '}';
    return dbg;
}

bool ChannelCondition::operator==(const ChannelCondition &rhs) const
{
    return this->twitchID == rhs.twitchID;
}

bool ChannelCondition::operator!=(const ChannelCondition &rhs) const
{
    return !(*this == rhs);
}

}  // namespace chatterino::seventv::eventapi
