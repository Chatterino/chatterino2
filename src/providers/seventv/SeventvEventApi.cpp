#include "providers/seventv/SeventvEventApi.hpp"

#include "providers/seventv/eventapi/SeventvEventApiClient.hpp"
#include "providers/seventv/eventapi/SeventvEventApiMessage.hpp"

#include <QJsonArray>
#include <utility>

namespace chatterino {

SeventvEventApi::SeventvEventApi(QString host,
                                 std::chrono::milliseconds heartbeatInterval)
    : BasicPubSubManager(std::move(host))
    , heartbeatInterval_(heartbeatInterval)
{
}

void SeventvEventApi::subscribeUser(const QString &userId,
                                    const QString &emoteSetId)
{
    if (!userId.isEmpty() && this->subscribedUsers_.insert(userId).second)
    {
        this->subscribe({userId, SeventvEventApiSubscriptionType::UpdateUser});
    }
    if (!emoteSetId.isEmpty() &&
        this->subscribedEmoteSets_.insert(emoteSetId).second)
    {
        this->subscribe(
            {emoteSetId, SeventvEventApiSubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventApi::unsubscribeEmoteSet(const QString &id)
{
    if (this->subscribedEmoteSets_.erase(id) > 0)
    {
        this->unsubscribe(
            {id, SeventvEventApiSubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventApi::unsubscribeUser(const QString &id)
{
    if (this->subscribedUsers_.erase(id) > 0)
    {
        this->unsubscribe({id, SeventvEventApiSubscriptionType::UpdateUser});
    }
}

std::shared_ptr<BasicPubSubClient<SeventvEventApiSubscription>>
    SeventvEventApi::createClient(liveupdates::WebsocketClient &client,
                                  websocketpp::connection_hdl hdl)
{
    auto shared = std::make_shared<SeventvEventApiClient>(
        client, hdl, this->heartbeatInterval_);
    return std::static_pointer_cast<
        BasicPubSubClient<SeventvEventApiSubscription>>(std::move(shared));
}

void SeventvEventApi::onMessage(
    websocketpp::connection_hdl hdl,
    BasicPubSubManager<SeventvEventApiSubscription>::WebsocketMessagePtr msg)
{
    const auto &payload = QString::fromStdString(msg->get_payload());

    auto pMessage = parseSeventvEventApiBaseMessage(payload);

    if (!pMessage)
    {
        qCDebug(chatterinoSeventvEventApi)
            << "Unable to parse incoming event-api message: " << payload;
        return;
    }
    qCDebug(chatterinoSeventvEventApi) << payload;
    auto message = *pMessage;
    switch (message.op)
    {
        case SeventvEventApiOpcode::Hello: {
            if (auto client = this->findClient(hdl))
            {
                if (auto *stvClient =
                        dynamic_cast<SeventvEventApiClient *>(client.get()))
                {
                    stvClient->setHeartbeatInterval(
                        message.data["heartbeat_interval"].toInt());
                }
            }
        }
        break;
        case SeventvEventApiOpcode::Heartbeat: {
            if (auto client = this->findClient(hdl))
            {
                if (auto *stvClient =
                        dynamic_cast<SeventvEventApiClient *>(client.get()))
                {
                    stvClient->handleHeartbeat();
                }
            }
        }
        break;
        case SeventvEventApiOpcode::Dispatch: {
            auto dispatch = message.toInner<SeventvEventApiDispatch>();
            if (!dispatch)
            {
                qCDebug(chatterinoSeventvEventApi)
                    << "Malformed dispatch" << payload;
                return;
            }
            this->handleDispatch(*dispatch);
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventApi) << "Unhandled op: " << payload;
        }
        break;
    }
}

void SeventvEventApi::handleDispatch(const SeventvEventApiDispatch &dispatch)
{
    switch (dispatch.type)
    {
        case SeventvEventApiSubscriptionType::UpdateEmoteSet: {
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

                SeventvEventApiEmoteAddDispatch added(
                    dispatch, pushed["value"].toObject());

                if (added.validate())
                {
                    this->signals_.emoteAdded.invoke(added);
                }
                else
                {
                    qCDebug(chatterinoSeventvEventApi)
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

                SeventvEventApiEmoteUpdateDispatch update(dispatch, updated);

                if (update.validate())
                {
                    this->signals_.emoteUpdated.invoke(update);
                }
                else
                {
                    qCDebug(chatterinoSeventvEventApi)
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

                SeventvEventApiEmoteRemoveDispatch removed(
                    dispatch, pulled["old_value"].toObject());

                if (removed.validate())
                {
                    this->signals_.emoteRemoved.invoke(removed);
                }
                else
                {
                    qCDebug(chatterinoSeventvEventApi)
                        << "Invalid dispatch" << dispatch.body;
                }
            }
        }
        break;
        case SeventvEventApiSubscriptionType::UpdateUser: {
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

                    SeventvEventApiUserConnectionUpdateDispatch update(dispatch,
                                                                       value);

                    if (update.validate())
                    {
                        this->signals_.userUpdated.invoke(update);
                    }
                    else
                    {
                        qCDebug(chatterinoSeventvEventApi)
                            << "Invalid dispatch" << dispatch.body;
                    }
                }
            }
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventApi)
                << "Unknown subscription type:" << (int)dispatch.type
                << "body:" << dispatch.body;
        }
        break;
    }
}

}  // namespace chatterino
