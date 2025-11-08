#pragma once

#include <atomic>

namespace chatterino::liveupdates {

struct Diag {
    std::atomic<uint32_t> connectionsClosed{0};
    std::atomic<uint32_t> connectionsOpened{0};
    std::atomic<uint32_t> connectionsFailed{0};
};

}  // namespace chatterino::liveupdates
