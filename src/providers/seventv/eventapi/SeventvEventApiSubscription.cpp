#include "providers/seventv/eventapi/SeventvEventApiSubscription.hpp"

#include <QJsonDocument>
#include <QJsonObject>

namespace {

using namespace chatterino;

const char *typeToString(SeventvEventApiSubscriptionType type)
{
    switch (type)
    {
        case SeventvEventApiSubscriptionType::UpdateEmoteSet:
            return "emote_set.update";
        case SeventvEventApiSubscriptionType::UpdateUser:
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

namespace chatterino {

bool SeventvEventApiSubscription::operator==(
    const SeventvEventApiSubscription &rhs) const
{
    return std::tie(this->condition, this->type) ==
           std::tie(rhs.condition, rhs.type);
}

bool SeventvEventApiSubscription::operator!=(
    const SeventvEventApiSubscription &rhs) const
{
    return !(rhs == *this);
}

QByteArray SeventvEventApiSubscription::encodeSubscribe() const
{
    const auto *typeName = typeToString(this->type);
    QJsonObject root;
    root["op"] = (int)SeventvEventApiOpcode::Subscribe;
    root["d"] = createDataJson(typeName, this->condition);
    return QJsonDocument(root).toJson();
}

QByteArray SeventvEventApiSubscription::encodeUnsubscribe() const
{
    const auto *typeName = typeToString(this->type);
    QJsonObject root;
    root["op"] = (int)SeventvEventApiOpcode::Unsubscribe;
    root["d"] = createDataJson(typeName, this->condition);
    return QJsonDocument(root).toJson();
}

QDebug &operator<<(QDebug &dbg, const SeventvEventApiSubscription &subscription)
{
    dbg << "SeventvEventApiSubscription{ condition:" << subscription.condition
        << "type:" << (int)subscription.type << '}';
    return dbg;
}

}  // namespace chatterino
