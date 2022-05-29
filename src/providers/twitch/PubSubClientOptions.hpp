#pragma once

#include <chrono>

namespace chatterino {

/**
 * @brief Options to change the behaviour of the underlying websocket clients
 **/
struct PubSubClientOptions {
    std::chrono::seconds pingInterval_;
};

}  // namespace chatterino
