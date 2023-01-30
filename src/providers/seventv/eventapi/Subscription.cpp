#include "providers/seventv/eventapi/Subscription.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include <tuple>
#include <utility>

namespace {

using namespace chatterino::seventv::eventapi;

const char *typeToString(SubscriptionType type)
{
    return magic_enum::enum_name(type).data();
}

QJsonObject createDataJson(const char *typeName, const Condition &condition)
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
    const auto *typeName = typeToString(this->type);
    QJsonObject root;
    root["op"] = (int)Opcode::Subscribe;
    root["d"] = createDataJson(typeName, this->condition);
    return QJsonDocument(root).toJson();
}

QByteArray Subscription::encodeUnsubscribe() const
{
    const auto *typeName = typeToString(this->type);
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
                << "type:" << magic_enum::enum_name(subscription.type).data()
                << '}';
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

}  // namespace chatterino::seventv::eventapi
