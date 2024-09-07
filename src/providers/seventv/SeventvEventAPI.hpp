#pragma once

#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"
#include "providers/seventv/eventapi/Subscription.hpp"
#include "util/QStringHash.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {

namespace seventv::eventapi {
    struct Dispatch;
    struct EmoteAddDispatch;
    struct EmoteUpdateDispatch;
    struct EmoteRemoveDispatch;
    struct UserConnectionUpdateDispatch;
    struct CosmeticCreateDispatch;
    struct EntitlementCreateDeleteDispatch;
}  // namespace seventv::eventapi

class SeventvBadges;

class SeventvEventAPI
    : public BasicPubSubManager<seventv::eventapi::Subscription>
{
    template <typename T>
    using Signal =
        pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

public:
    SeventvEventAPI(QString host,
                    std::chrono::milliseconds defaultHeartbeatInterval =
                        std::chrono::milliseconds(25000));

    ~SeventvEventAPI() override;

    struct {
        Signal<seventv::eventapi::EmoteAddDispatch> emoteAdded;
        Signal<seventv::eventapi::EmoteUpdateDispatch> emoteUpdated;
        Signal<seventv::eventapi::EmoteRemoveDispatch> emoteRemoved;
        Signal<seventv::eventapi::UserConnectionUpdateDispatch> userUpdated;
    } signals_;  // NOLINT(readability-identifier-naming)

    /**
     * Subscribes to a user and emote-set
     * if not already subscribed.
     *
     * @param userID 7TV user-id, may be empty.
     * @param emoteSetID 7TV emote-set-id, may be empty.
     */
    void subscribeUser(const QString &userID, const QString &emoteSetID);
    /**
     * Subscribes to cosmetics and entitlements in a Twitch channel
     * if not already subscribed.
     *
     * @param id Twitch channel id
     */
    void subscribeTwitchChannel(const QString &id);

    /** Unsubscribes from a user by its 7TV user id */
    void unsubscribeUser(const QString &id);
    /** Unsubscribes from an emote-set by its id */
    void unsubscribeEmoteSet(const QString &id);
    /** Unsubscribes from cosmetics and entitlements in a Twitch channel */
    void unsubscribeTwitchChannel(const QString &id);

protected:
    std::shared_ptr<BasicPubSubClient<seventv::eventapi::Subscription>>
        createClient(liveupdates::WebsocketClient &client,
                     websocketpp::connection_hdl hdl) override;
    void onMessage(
        websocketpp::connection_hdl hdl,
        BasicPubSubManager<seventv::eventapi::Subscription>::WebsocketMessagePtr
            msg) override;

private:
    void handleDispatch(const seventv::eventapi::Dispatch &dispatch);

    void onEmoteSetUpdate(const seventv::eventapi::Dispatch &dispatch);
    void onUserUpdate(const seventv::eventapi::Dispatch &dispatch);
    void onCosmeticCreate(
        const seventv::eventapi::CosmeticCreateDispatch &cosmetic);
    void onEntitlementCreate(
        const seventv::eventapi::EntitlementCreateDeleteDispatch &entitlement);
    void onEntitlementDelete(
        const seventv::eventapi::EntitlementCreateDeleteDispatch &entitlement);

    /** emote-set ids */
    std::unordered_set<QString> subscribedEmoteSets_;
    /** user ids */
    std::unordered_set<QString> subscribedUsers_;
    /** Twitch channel ids */
    std::unordered_set<QString> subscribedTwitchChannels_;
    std::chrono::milliseconds heartbeatInterval_;
};

}  // namespace chatterino
