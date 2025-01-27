#!/usr/bin/env bash

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

subscription_name="$1"
subscription_version="$2"

usage() {
    >&2 echo "Usage: $0 <name> <version> (e.g. $0 channel.ban v1)"
    exit 1
}

if [ -z "$subscription_name" ]; then
    >&2 echo "Missing subscription name"
    usage
fi

if [ -z "$subscription_version" ]; then
    >&2 echo "Missing subscription version"
    usage
fi

# Clean up subscription name

dashed_subscription_name="$(echo "$subscription_name" | sed 's/\./-/g')"
underscored_subscription_name="$(echo "$subscription_name" | sed 's/\./_/g')"
echo "Subscription name: $subscription_name ($dashed_subscription_name)"
echo "Subscription version: $subscription_version"

header_file_name="${dashed_subscription_name}-${subscription_version}.hpp"
source_file_name="${dashed_subscription_name}-${subscription_version}.cpp"

# Write the header file
cat > "$SCRIPT_DIR/../../include/twitch-eventsub-ws/payloads/$header_file_name" << EOF
#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::$underscored_subscription_name::$subscription_version {

/// json_transform=snake_case
struct Event {
    // TODO: Fill in your subscription-specific event here
};

struct Payload {
    const subscription::Subscription subscription;

    const Event event;
};

// DESERIALIZATION DEFINITION START
// DESERIALIZATION DEFINITION END

}  // namespace chatterino::eventsub::lib::payload::$underscored_subscription_name::$subscription_version
EOF

# Write the source file
cat > "$SCRIPT_DIR/$source_file_name" << EOF
#include "twitch-eventsub-ws/payloads/$header_file_name"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::$underscored_subscription_name::$subscription_version {

// DESERIALIZATION IMPLEMENTATION START
// DESERIALIZATION IMPLEMENTATION END

}  // namespace chatterino::eventsub::lib::payload::$underscored_subscription_name::$subscription_version
EOF

echo "Steps that are left for you:"
echo "1. Add a virtual method to the Listener class in include/twitch-eventsub-ws/listener.hpp"
echo "2. Add a handler for this subscription in src/session.cpp's NOTIFICATION_HANDLERS"
echo "3. Add payloads/${source_file_name} to src/CMakeLists.txt"
