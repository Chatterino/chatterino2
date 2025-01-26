#pragma once

#include "providers/twitch/eventsub/SubscriptionRequest.hpp"

#include <memory>

namespace chatterino::eventsub {

struct RawSubscriptionHandle {
    const SubscriptionRequest request;

    RawSubscriptionHandle(SubscriptionRequest request_);

    ~RawSubscriptionHandle();
};

using SubscriptionHandle = std::unique_ptr<RawSubscriptionHandle>;

}  // namespace chatterino::eventsub
