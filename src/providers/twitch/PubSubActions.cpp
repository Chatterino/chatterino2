#include "providers/twitch/PubSubActions.hpp"

namespace chatterino {

PubSubAction::PubSubAction(const QJsonObject &data, const QString &_roomID)
    : timestamp(std::chrono::steady_clock::now())
    , roomID(_roomID)
{
    this->source.id = data.value("created_by_user_id").toString();
    this->source.login = data.value("created_by").toString();
}

}  // namespace chatterino
