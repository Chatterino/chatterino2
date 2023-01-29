#include "providers/seventv/eventapi/SeventvEventAPISubscription.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include <tuple>

namespace {

using namespace chatterino;

const char *typeToString(SeventvEventAPISubscriptionType type)
{
    switch (type)
    {
        case SeventvEventAPISubscriptionType::UpdateEmoteSet:
            return "emote_set.update";
        case SeventvEventAPISubscriptionType::UpdateUser:
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

bool SeventvEventAPISubscription::operator==(
    const SeventvEventAPISubscription &rhs) const
{
    return std::tie(this->condition, this->type) ==
           std::tie(rhs.condition, rhs.type);
}

bool SeventvEventAPISubscription::operator!=(
    const SeventvEventAPISubscription &rhs) const
{
    return !(rhs == *this);
}

QByteArray SeventvEventAPISubscription::encodeSubscribe() const
{
    const auto *typeName = typeToString(this->type);
    QJsonObject root;
    root["op"] = (int)SeventvEventAPIOpcode::Subscribe;
    root["d"] = createDataJson(typeName, this->condition);
    return QJsonDocument(root).toJson();
}

QByteArray SeventvEventAPISubscription::encodeUnsubscribe() const
{
    const auto *typeName = typeToString(this->type);
    QJsonObject root;
    root["op"] = (int)SeventvEventAPIOpcode::Unsubscribe;
    root["d"] = createDataJson(typeName, this->condition);
    return QJsonDocument(root).toJson();
}

QDebug &operator<<(QDebug &dbg, const SeventvEventAPISubscription &subscription)
{
    dbg << "SeventvEventAPISubscription{ condition:" << subscription.condition
        << "type:" << (int)subscription.type << '}';
    return dbg;
}

}  // namespace chatterino
