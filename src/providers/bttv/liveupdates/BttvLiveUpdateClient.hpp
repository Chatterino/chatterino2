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

private:
    BttvLiveUpdates &manager;
};

}  // namespace chatterino
