#include "providers/seventv/eventapi/Client.hpp"

#include "Application.hpp"
#include "providers/seventv/eventapi/Dispatch.hpp"
#include "providers/seventv/eventapi/Message.hpp"
#include "providers/seventv/eventapi/Subscription.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/seventv/SeventvEventAPI.hpp"
#include "util/QMagicEnum.hpp"

#include <QJsonArray>

namespace chatterino::seventv::eventapi {

Client::Client(SeventvEventAPI &manager,
               std::chrono::milliseconds heartbeatInterval)
    : BasicPubSubClient<Subscription>(100)
    , lastHeartbeat_(std::chrono::steady_clock::now())
    , heartbeatInterval_(heartbeatInterval)
    , manager_(manager)
{
}

void Client::onOpen()
{
    BasicPubSubClient<Subscription>::onOpen();
    this->lastHeartbeat_.store(std::chrono::steady_clock::now(),
                               std::memory_order::relaxed);
}

void Client::onMessage(const QByteArray &msg)
{
    auto pMessage = parseBaseMessage(msg);

    if (!pMessage)
    {
        qCDebug(chatterinoSeventvEventAPI)
            << "Unable to parse incoming event-api message: " << msg;
        return;
    }
    auto message = *pMessage;
    switch (message.op)
    {
        case Opcode::Hello: {
            this->heartbeatInterval_.store(
                std::chrono::milliseconds{
                    message.data["heartbeat_interval"].toInt()},
                std::memory_order::relaxed);
        }
        break;
        case Opcode::Heartbeat: {
            this->lastHeartbeat_.store(std::chrono::steady_clock::now(),
                                       std::memory_order::relaxed);
        }
        break;
        case Opcode::Dispatch: {
            auto dispatch = message.toInner<Dispatch>();
            if (!dispatch)
            {
                qCDebug(chatterinoSeventvEventAPI)
                    << "Malformed dispatch" << msg;
                return;
            }
            this->handleDispatch(*dispatch);
        }
        break;
        case Opcode::Reconnect: {
            this->close();
        }
        break;
        case Opcode::Ack: {
            // unhandled
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventAPI) << "Unhandled op:" << msg;
        }
        break;
    }
}

void Client::checkHeartbeat()
{
    if ((std::chrono::steady_clock::now() -
         this->lastHeartbeat_.load(std::memory_order::relaxed)) >
        this->heartbeatInterval() * 3)
    {
        qCDebug(chatterinoSeventvEventAPI) << "Hearbeat timed out";
        this->close();
    }
}

std::chrono::milliseconds Client::heartbeatInterval() const
{
    return this->heartbeatInterval_.load(std::memory_order::relaxed);
}

void Client::handleDispatch(const Dispatch &dispatch)
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
        // NOLINTNEXTLINE(bugprone-branch-clone)
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

void Client::onEmoteSetUpdate(const Dispatch &dispatch)
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
            this->manager_.signals_.emoteAdded.invoke(added);
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
            this->manager_.signals_.emoteUpdated.invoke(update);
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
            this->manager_.signals_.emoteRemoved.invoke(removed);
        }
        else
        {
            qCDebug(chatterinoSeventvEventAPI)
                << "Invalid dispatch" << dispatch.body;
        }
    }
}

void Client::onUserUpdate(const Dispatch &dispatch)
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
                this->manager_.signals_.userUpdated.invoke(update);
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

void Client::onCosmeticCreate(const CosmeticCreateDispatch &cosmetic)
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

void Client::onEntitlementCreate(
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

void Client::onEntitlementDelete(
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

}  // namespace chatterino::seventv::eventapi
