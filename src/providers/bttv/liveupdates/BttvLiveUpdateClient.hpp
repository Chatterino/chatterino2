#pragma once

#include "providers/bttv/liveupdates/BttvLiveUpdateSubscription.hpp"
#include "providers/liveupdates/BasicPubSubClient.hpp"

namespace chatterino {

class BttvLiveUpdates;

class BttvLiveUpdateClient
    : public BasicPubSubClient<BttvLiveUpdateSubscription>
{
public:
    BttvLiveUpdateClient(BttvLiveUpdates &manager);

    void onMessage(const QByteArray &msg) /* override */;

    void broadcastMe(const QString &channelID, const QString &userID);

private:
    BttvLiveUpdates &manager;
};

}  // namespace chatterino
