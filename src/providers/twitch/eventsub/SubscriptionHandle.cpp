// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/twitch/eventsub/SubscriptionHandle.hpp"

#include "Application.hpp"
#include "providers/twitch/eventsub/Controller.hpp"

namespace chatterino::eventsub {

RawSubscriptionHandle::RawSubscriptionHandle(SubscriptionRequest request_)
    : request(std::move(request_))
{
    // The reference is added by the EventSub controller
}

RawSubscriptionHandle::~RawSubscriptionHandle()
{
    auto *app = tryGetApp();
    if (app == nullptr)
    {
        // We're shutting down, assume the unsubscription has been taken care of
        return;
    }
    app->getEventSub()->removeRef(this->request);
}

}  // namespace chatterino::eventsub
