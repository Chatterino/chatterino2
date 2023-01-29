#pragma once

#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"
#include "util/QStringHash.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {

struct SeventvEventAPISubscription;
struct SeventvEventAPIDispatch;
struct SeventvEventAPIEmoteAddDispatch;
struct SeventvEventAPIEmoteUpdateDispatch;
struct SeventvEventAPIEmoteRemoveDispatch;
struct SeventvEventAPIUserConnectionUpdateDispatch;

class SeventvEventAPI : public BasicPubSubManager<SeventvEventAPISubscription>
{
    template <typename T>
    using Signal =
        pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

public:
    SeventvEventAPI(QString host,
                    std::chrono::milliseconds defaultHeartbeatInterval =
                        std::chrono::milliseconds(25000));

    struct {
        Signal<SeventvEventAPIEmoteAddDispatch> emoteAdded;
        Signal<SeventvEventAPIEmoteUpdateDispatch> emoteUpdated;
        Signal<SeventvEventAPIEmoteRemoveDispatch> emoteRemoved;
        Signal<SeventvEventAPIUserConnectionUpdateDispatch> userUpdated;
    } signals_;  // NOLINT(readability-identifier-naming)

    /**
     * Subscribes to a user and emote-set
     * if not already subscribed.
     *
     * @param userID 7TV user-id, may be empty.
     * @param emoteSetID 7TV emote-set-id, may be empty.
     */
    void subscribeUser(const QString &userID, const QString &emoteSetID);

    /** Unsubscribes from a user by its 7TV user id */
    void unsubscribeUser(const QString &id);
    /** Unsubscribes from an emote-set by its id */
    void unsubscribeEmoteSet(const QString &id);

protected:
    std::shared_ptr<BasicPubSubClient<SeventvEventAPISubscription>>
        createClient(liveupdates::WebsocketClient &client,
                     websocketpp::connection_hdl hdl) override;
    void onMessage(
        websocketpp::connection_hdl hdl,
        BasicPubSubManager<SeventvEventAPISubscription>::WebsocketMessagePtr
            msg) override;

private:
    void handleDispatch(const SeventvEventAPIDispatch &dispatch);

    std::unordered_set<QString> subscribedEmoteSets_;
    std::unordered_set<QString> subscribedUsers_;
    std::chrono::milliseconds heartbeatInterval_;
};

}  // namespace chatterino
