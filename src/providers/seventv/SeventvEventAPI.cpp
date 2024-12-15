#include "providers/seventv/SeventvEventAPI.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "providers/seventv/eventapi/Client.hpp"
#include "providers/seventv/eventapi/Dispatch.hpp"
#include "providers/seventv/eventapi/Message.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/seventv/SeventvCosmetics.hpp"
#include "util/QMagicEnum.hpp"

#include <QJsonArray>

#include <utility>

namespace chatterino {

using namespace seventv;
using namespace seventv::eventapi;
using namespace chatterino::literals;

SeventvEventAPI::SeventvEventAPI(
    QString host, std::chrono::milliseconds defaultHeartbeatInterval)
    : BasicPubSubManager(std::move(host), u"7TV"_s)
    , heartbeatInterval_(defaultHeartbeatInterval)
{
}

SeventvEventAPI::~SeventvEventAPI()
{
    this->stop();
}

void SeventvEventAPI::subscribeUser(const QString &userID,
                                    const QString &emoteSetID)
{
    if (!userID.isEmpty() && this->subscribedUsers_.insert(userID).second)
    {
        this->subscribe(
            {ObjectIDCondition{userID}, SubscriptionType::UpdateUser});
    }
    if (!emoteSetID.isEmpty() &&
        this->subscribedEmoteSets_.insert(emoteSetID).second)
    {
        this->subscribe(
            {ObjectIDCondition{emoteSetID}, SubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventAPI::subscribeTwitchChannel(const QString &id)
{
    if (this->subscribedTwitchChannels_.insert(id).second)
    {
        this->subscribe({
            ChannelCondition{id},
            SubscriptionType::CreateCosmetic,
        });
        this->subscribe({
            ChannelCondition{id},
            SubscriptionType::CreateEntitlement,
        });
        this->subscribe({
            ChannelCondition{id},
            SubscriptionType::DeleteEntitlement,
        });
    }
}

void SeventvEventAPI::unsubscribeEmoteSet(const QString &id)
{
    if (this->subscribedEmoteSets_.erase(id) > 0)
    {
        this->unsubscribe(
            {ObjectIDCondition{id}, SubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventAPI::unsubscribeUser(const QString &id)
{
    if (this->subscribedUsers_.erase(id) > 0)
    {
        this->unsubscribe(
            {ObjectIDCondition{id}, SubscriptionType::UpdateUser});
    }
}

void SeventvEventAPI::unsubscribeTwitchChannel(const QString &id)
{
    if (this->subscribedTwitchChannels_.erase(id) > 0)
    {
        this->unsubscribe({
            ChannelCondition{id},
            SubscriptionType::CreateCosmetic,
        });
        this->unsubscribe({
            ChannelCondition{id},
            SubscriptionType::CreateEntitlement,
        });
        this->unsubscribe({
            ChannelCondition{id},
            SubscriptionType::DeleteEntitlement,
        });
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
        case Opcode::Ack: {
            // unhandled
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventAPI) << "Unhandled op:" << payload;
        }
        break;
    }
}

void SeventvEventAPI::handleDispatch(const Dispatch &dispatch)
{
    switch (dispatch.type)
    {
        case SubscriptionType::UpdateEmoteSet: {
            this->onEmoteSetUpdate(dispatch);
        }
        break;
        case SubscriptionType::UpdateUser: {
            this->onUserUpdate(dispatch);
        }
        break;
        case SubscriptionType::CreateCosmetic: {
            const CosmeticCreateDispatch cosmetic(dispatch);
            if (cosmetic.validate())
            {
                this->onCosmeticCreate(cosmetic);
            }
            else
            {
                qCDebug(chatterinoSeventvEventAPI)
                    << "Invalid cosmetic dispatch" << dispatch.body;
            }
        }
        break;
        case SubscriptionType::CreateEntitlement: {
            const EntitlementCreateDeleteDispatch entitlement(dispatch);
            if (entitlement.validate())
            {
                this->onEntitlementCreate(entitlement);
            }
            else
            {
                qCDebug(chatterinoSeventvEventAPI)
                    << "Invalid entitlement create dispatch" << dispatch.body;
            }
        }
        break;
        case SubscriptionType::DeleteEntitlement: {
            const EntitlementCreateDeleteDispatch entitlement(dispatch);
            if (entitlement.validate())
            {
                this->onEntitlementDelete(entitlement);
            }
            else
            {
                qCDebug(chatterinoSeventvEventAPI)
                    << "Invalid entitlement delete dispatch" << dispatch.body;
            }
        }
        break;
        case SubscriptionType::ResetEntitlement: {
            // unhandled (not clear what we'd do here yet)
        }
        break;
        case SubscriptionType::CreateEmoteSet: {
            // unhandled (c2 does not support custom emote sets)
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventAPI)
                << "Unknown subscription type:"
                << qmagicenum::enumName(dispatch.type)
                << "body:" << dispatch.body;
        }
        break;
    }
}

void SeventvEventAPI::onEmoteSetUpdate(const Dispatch &dispatch)
{
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

        const EmoteAddDispatch added(dispatch, pushed["value"].toObject());

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

        const EmoteUpdateDispatch update(dispatch,
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

        const EmoteRemoveDispatch removed(dispatch,
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

void SeventvEventAPI::onUserUpdate(const Dispatch &dispatch)
{
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

            const UserConnectionUpdateDispatch update(
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

// NOLINTBEGIN(readability-convert-member-functions-to-static)

void SeventvEventAPI::onCosmeticCreate(const CosmeticCreateDispatch &cosmetic)
{
    auto *app = tryGetApp();
    if (!app)
    {
        return;  // shutting down
    }

    auto *badges = app->getSeventvBadges();
    switch (cosmetic.kind)
    {
        case CosmeticKind::Badge: {
            badges->registerBadge(cosmetic.data);
        }
        break;
        default:
            break;
    }
}

void SeventvEventAPI::onEntitlementCreate(
    const EntitlementCreateDeleteDispatch &entitlement)
{
    auto *app = tryGetApp();
    if (!app)
    {
        return;  // shutting down
    }

    auto *badges = app->getSeventvBadges();
    switch (entitlement.kind)
    {
        case CosmeticKind::Badge: {
            badges->assignBadgeToUser(entitlement.refID,
                                      UserId{entitlement.userID});
        }
        break;
        default:
            break;
    }
}

void SeventvEventAPI::onEntitlementDelete(
    const EntitlementCreateDeleteDispatch &entitlement)
{
    auto *app = tryGetApp();
    if (!app)
    {
        return;  // shutting down
    }

    auto *badges = app->getSeventvBadges();
    switch (entitlement.kind)
    {
        case CosmeticKind::Badge: {
            badges->clearBadgeFromUser(entitlement.refID,
                                       UserId{entitlement.userID});
        }
        break;
        default:
            break;
    }
}
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace chatterino
