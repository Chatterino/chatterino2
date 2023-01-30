#include "providers/seventv/SeventvEventAPI.hpp"

#include "providers/seventv/eventapi/Client.hpp"
#include "providers/seventv/eventapi/Dispatch.hpp"
#include "providers/seventv/eventapi/Message.hpp"

#include <QJsonArray>

#include <utility>

namespace chatterino {
using namespace seventv::eventapi;

SeventvEventAPI::SeventvEventAPI(
    QString host, std::chrono::milliseconds defaultHeartbeatInterval)
    : BasicPubSubManager(std::move(host))
    , heartbeatInterval_(defaultHeartbeatInterval)
{
}

void SeventvEventAPI::subscribeUser(const QString &userID,
                                    const QString &emoteSetID)
{
    if (!userID.isEmpty() && this->subscribedUsers_.insert(userID).second)
    {
        this->subscribe({userID, SubscriptionType::UpdateUser});
    }
    if (!emoteSetID.isEmpty() &&
        this->subscribedEmoteSets_.insert(emoteSetID).second)
    {
        this->subscribe({emoteSetID, SubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventAPI::unsubscribeEmoteSet(const QString &id)
{
    if (this->subscribedEmoteSets_.erase(id) > 0)
    {
        this->unsubscribe({id, SubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventAPI::unsubscribeUser(const QString &id)
{
    if (this->subscribedUsers_.erase(id) > 0)
    {
        this->unsubscribe({id, SubscriptionType::UpdateUser});
    }
}

std::shared_ptr<BasicPubSubClient<Subscription>> SeventvEventAPI::createClient(
    liveupdates::WebsocketClient &client, websocketpp::connection_hdl hdl)
{
    auto shared =
        std::make_shared<Client>(client, hdl, this->heartbeatInterval_);
    return std::static_pointer_cast<BasicPubSubClient<Subscription>>(
        std::move(shared));
}

void SeventvEventAPI::onMessage(
    websocketpp::connection_hdl hdl,
    BasicPubSubManager<Subscription>::WebsocketMessagePtr msg)
{
    const auto &payload = QString::fromStdString(msg->get_payload());

    auto pMessage = parseBaseMessage(payload);

    if (!pMessage)
    {
        qCDebug(chatterinoSeventvEventAPI)
            << "Unable to parse incoming event-api message: " << payload;
        return;
    }
    auto message = *pMessage;
    switch (message.op)
    {
        case Opcode::Hello: {
            if (auto client = this->findClient(hdl))
            {
                if (auto *stvClient = dynamic_cast<Client *>(client.get()))
                {
                    stvClient->setHeartbeatInterval(
                        message.data["heartbeat_interval"].toInt());
                }
            }
        }
        break;
        case Opcode::Heartbeat: {
            if (auto client = this->findClient(hdl))
            {
                if (auto *stvClient = dynamic_cast<Client *>(client.get()))
                {
                    stvClient->handleHeartbeat();
                }
            }
        }
        break;
        case Opcode::Dispatch: {
            auto dispatch = message.toInner<Dispatch>();
            if (!dispatch)
            {
                qCDebug(chatterinoSeventvEventAPI)
                    << "Malformed dispatch" << payload;
                return;
            }
            this->handleDispatch(*dispatch);
        }
        break;
        case Opcode::Reconnect: {
            if (auto client = this->findClient(hdl))
            {
                if (auto *stvClient = dynamic_cast<Client *>(client.get()))
                {
                    stvClient->close("Reconnecting");
                }
            }
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventAPI) << "Unhandled op: " << payload;
        }
        break;
    }
}

void SeventvEventAPI::handleDispatch(const Dispatch &dispatch)
{
    switch (dispatch.type)
    {
        case SubscriptionType::UpdateEmoteSet: {
            // dispatchBody: {
            //   pushed:  Array<{ key, value            }>,
            //   pulled:  Array<{ key,        old_value }>,
            //   updated: Array<{ key, value, old_value }>,
            // }
            for (const auto pushedRef : dispatch.body["pushed"].toArray())
            {
                auto pushed = pushedRef.toObject();
                if (pushed["key"].toString() != "emotes")
                {
                    continue;
                }

                EmoteAddDispatch added(dispatch, pushed["value"].toObject());

                if (added.validate())
                {
                    this->signals_.emoteAdded.invoke(added);
                }
                else
                {
                    qCDebug(chatterinoSeventvEventAPI)
                        << "Invalid dispatch" << dispatch.body;
                }
            }
            for (const auto updatedRef : dispatch.body["updated"].toArray())
            {
                auto updated = updatedRef.toObject();
                if (updated["key"].toString() != "emotes")
                {
                    continue;
                }

                EmoteUpdateDispatch update(dispatch,
                                           updated["old_value"].toObject(),
                                           updated["value"].toObject());

                if (update.validate())
                {
                    this->signals_.emoteUpdated.invoke(update);
                }
                else
                {
                    qCDebug(chatterinoSeventvEventAPI)
                        << "Invalid dispatch" << dispatch.body;
                }
            }
            for (const auto pulledRef : dispatch.body["pulled"].toArray())
            {
                auto pulled = pulledRef.toObject();
                if (pulled["key"].toString() != "emotes")
                {
                    continue;
                }

                EmoteRemoveDispatch removed(dispatch,
                                            pulled["old_value"].toObject());

                if (removed.validate())
                {
                    this->signals_.emoteRemoved.invoke(removed);
                }
                else
                {
                    qCDebug(chatterinoSeventvEventAPI)
                        << "Invalid dispatch" << dispatch.body;
                }
            }
        }
        break;
        case SubscriptionType::UpdateUser: {
            // dispatchBody: {
            //   updated: Array<{ key, value: Array<{key, value}> }>
            // }
            for (const auto updatedRef : dispatch.body["updated"].toArray())
            {
                auto updated = updatedRef.toObject();
                if (updated["key"].toString() != "connections")
                {
                    continue;
                }
                for (const auto valueRef : updated["value"].toArray())
                {
                    auto value = valueRef.toObject();
                    if (value["key"].toString() != "emote_set")
                    {
                        continue;
                    }

                    UserConnectionUpdateDispatch update(
                        dispatch, value, (size_t)updated["index"].toInt());

                    if (update.validate())
                    {
                        this->signals_.userUpdated.invoke(update);
                    }
                    else
                    {
                        qCDebug(chatterinoSeventvEventAPI)
                            << "Invalid dispatch" << dispatch.body;
                    }
                }
            }
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventAPI)
                << "Unknown subscription type:" << (int)dispatch.type
                << "body:" << dispatch.body;
        }
        break;
    }
}

}  // namespace chatterino
