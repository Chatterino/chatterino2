#pragma once

#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"
#include "providers/seventv/eventapi/SeventvEventApiDispatch.hpp"
#include "providers/seventv/eventapi/SeventvEventApiSubscription.hpp"
#include "util/QStringHash.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {

class SeventvEventApi : public BasicPubSubManager<SeventvEventApiSubscription>
{
    template <typename T>
    using Signal =
        pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

public:
    SeventvEventApi(QString host, std::chrono::milliseconds heartbeatInterval =
                                      std::chrono::milliseconds(25000));

    struct {
        Signal<SeventvEventApiEmoteAddDispatch> emoteAdded;
        Signal<SeventvEventApiEmoteUpdateDispatch> emoteUpdated;
        Signal<SeventvEventApiEmoteRemoveDispatch> emoteRemoved;
        Signal<SeventvEventApiUserConnectionUpdateDispatch> userUpdated;
    } signals_;  // NOLINT(readability-identifier-naming)

    void subscribeUser(const QString &userId, const QString &emoteSetId);
    void unsubscribeUser(const QString &id);
    void unsubscribeEmoteSet(const QString &id);

protected:
    std::shared_ptr<BasicPubSubClient<SeventvEventApiSubscription>>
        createClient(liveupdates::WebsocketClient &client,
                     websocketpp::connection_hdl hdl) override;
    void onMessage(
        websocketpp::connection_hdl hdl,
        BasicPubSubManager<SeventvEventApiSubscription>::WebsocketMessagePtr
            msg) override;

private:
    void handleDispatch(const SeventvEventApiDispatch &dispatch);

    std::unordered_set<QString> subscribedEmoteSets_;
    std::unordered_set<QString> subscribedUsers_;
    std::chrono::milliseconds heartbeatInterval_;
};

}  // namespace chatterino
