// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "providers/bttv/liveupdates/BttvLiveUpdateSubscription.hpp"
#include "providers/liveupdates/BasicPubSubClient.hpp"

namespace chatterino {

class BttvLiveUpdates;

class BttvLiveUpdateClient
    : public BasicPubSubClient<BttvLiveUpdateSubscription, BttvLiveUpdateClient>
{
public:
    BttvLiveUpdateClient(BttvLiveUpdates &manager);

    void onMessage(const QByteArray &msg) /* override */;

    void broadcastMe(const QString &channelID, const QString &userID);

private:
    BttvLiveUpdates &manager;
};

}  // namespace chatterino
