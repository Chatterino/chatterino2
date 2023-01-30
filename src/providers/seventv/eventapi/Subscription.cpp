#include "providers/seventv/eventapi/Subscription.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include <tuple>

namespace {

using namespace chatterino::seventv::eventapi;

const char *typeToString(SubscriptionType type)
{
    switch (type)
    {
        case SubscriptionType::UpdateEmoteSet:
            return "emote_set.update";
        case SubscriptionType::UpdateUser:
            return "user.update";
        default:
            return "";
    }
}

QJsonObject createDataJson(const char *typeName, const QString &condition)
{
    QJsonObject data;
    data["type"] = typeName;
    {
        QJsonObject conditionObj;
        conditionObj["object_id"] = condition;
        data["condition"] = conditionObj;
    }
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
    dbg << "7TV-Subscription{ condition:" << subscription.condition
        << "type:" << (int)subscription.type << '}';
    return dbg;
}

}  // namespace chatterino::seventv::eventapi
