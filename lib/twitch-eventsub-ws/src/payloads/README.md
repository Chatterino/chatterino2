# Adding a new subscription type

The example will use `channel.update` on version `1`.

## Create the header & source files

Replace `channel-update`/`channel_update` with your dashed & underscored subscription names

Header file `include/twitch-eventsub-ws/payloads/channel-update-v1.hpp`:

```c++
#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::channel_update::v1 {

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

}  // namespace chatterino::eventsub::lib::payload::channel_update::v1
```

Source file:

```c++
#include "twitch-eventsub-ws/payloads/channel-update-v1.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::channel_update::v1 {

// DESERIALIZATION IMPLEMENTATION START
// DESERIALIZATION IMPLEMENTATION END

}  // namespace chatterino::eventsub::lib::payload::channel_update::v1
```

## Run the generation script

In the root dir of the repostitory, type `make` in your terminal.

If you can't do that, run the `generate-and-replace-dir.py` script manually (e.g. `./ast/generate-and-replace-dir.py ./src/payloads`)

## Add the source file to `src/CMakeLists.txt`

Look for the `# Add your new subscription type source file above this line` comment and add the source file above that line.

In my example, I added the following line:  
`payloads/channel-update-v1.cpp`

## Add a virtual method to the Listener class in `include/twitch-eventsub-ws/listener.hpp`

Look for the `// Add your new subscription types above this line` comment and add your definition above that line.

In my example, I added the following code:

```c++
    virtual void onChannelUpdate(
        messages::Metadata metadata,
        payload::channel_update::v1::Payload payload) = 0;
```

You also need to add an include to your header file

In my example, I added the following code at the top of the file:

```c++
#include "twitch-eventsub-ws/payloads/channel-update-v1.hpp"
```

## Add a handler for the subscription type in `src/session.cpp`

Look for the `// Add your new subscription types above this line` comment and add your code above that line.

In my example, I added the following code:

```c++
    {
        {"channel.update", "1"},
        [](const auto &metadata, const auto &jv, auto &listener) {
            auto oPayload =
                parsePayload<eventsub::payload::channel_update::v1::Payload>(
                    jv);
            if (!oPayload)
            {
                return;
            }
            listener->onChannelUpdate(metadata, *oPayload);
        },
    },
```

## Make the test code work in `example/main.cpp`

Look for the `// Add your new subscription types above this line` comment and add your code above that line.

In my example, I added the following code:

```c++
    void onChannelUpdate(messages::Metadata metadata,
                         payload::channel_update::v1::Payload payload) override
    {
        std::cout << "Channel update event!\n";
    }
```

Full PR that does this: https://github.com/pajlada/beast-websocket-client/pull/19/files

TODO: Add step for how to verify this using the Twitch CLI, realistically though this will differ from subscription to subscription. Some are simpler than others
