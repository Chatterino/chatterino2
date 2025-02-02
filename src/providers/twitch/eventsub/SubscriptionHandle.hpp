#pragma once

#include "providers/twitch/eventsub/SubscriptionRequest.hpp"

#include <memory>

namespace chatterino::eventsub {

struct RawSubscriptionHandle {
    const SubscriptionRequest request;

    RawSubscriptionHandle(SubscriptionRequest request_);

    ~RawSubscriptionHandle();
};

/// Keeps a reference count of a specific subscription
///
/// If no more references exist of a specific subscription, we send an
/// unsubscription request
using SubscriptionHandle = std::unique_ptr<RawSubscriptionHandle>;

}  // namespace chatterino::eventsub
