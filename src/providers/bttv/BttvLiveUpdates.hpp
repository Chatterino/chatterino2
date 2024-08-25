#pragma once

#include "providers/bttv/liveupdates/BttvLiveUpdateMessages.hpp"
#include "providers/bttv/liveupdates/BttvLiveUpdateSubscription.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"
#include "util/QStringHash.hpp"

#include <pajlada/signals/signal.hpp>

#include <unordered_set>

namespace chatterino {

class BttvLiveUpdates : public BasicPubSubManager<BttvLiveUpdateSubscription>
{
    template <typename T>
    using Signal =
        pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

public:
    BttvLiveUpdates(QString host);
    ~BttvLiveUpdates() override;

    struct {
        Signal<BttvLiveUpdateEmoteUpdateAddMessage> emoteAdded;
        Signal<BttvLiveUpdateEmoteUpdateAddMessage> emoteUpdated;
        Signal<BttvLiveUpdateEmoteRemoveMessage> emoteRemoved;
    } signals_;  // NOLINT(readability-identifier-naming)

    /**
     * Joins a Twitch channel by its id (without any prefix like 'twitch:')
     * if it's not already joined.
     *
     * @param channelID the Twitch channel-id of the broadcaster.
     * @param userName the Twitch username of the current user.
     */
    void joinChannel(const QString &channelID, const QString &userName);

    /**
     * Parts a twitch channel by its id (without any prefix like 'twitch:')
     * if it's joined.
     *
     * @param id the Twitch channel-id of the broadcaster.
     */
    void partChannel(const QString &id);

protected:
    void onMessage(
        websocketpp::connection_hdl hdl,
        BasicPubSubManager<BttvLiveUpdateSubscription>::WebsocketMessagePtr msg)
        override;

private:
    // Contains all joined Twitch channel-ids
    std::unordered_set<QString> joinedChannels_;
};

}  // namespace chatterino
