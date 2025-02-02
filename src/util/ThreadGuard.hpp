#pragma once

#include <cassert>
#include <mutex>
#include <optional>
#include <thread>

namespace chatterino {

// Debug-class which asserts if guard of the same object has been called from different threads
struct ThreadGuard {
#ifndef NDEBUG
    mutable std::mutex mutex;
    mutable std::optional<std::thread::id> threadID;
#endif

    ThreadGuard() = default;

    explicit ThreadGuard(std::thread::id threadID_)
#ifndef NDEBUG
        : threadID(threadID_)
#endif
    {
    }

    inline void guard() const
    {
#ifndef NDEBUG
        std::unique_lock lock(this->mutex);

        auto currentThreadID = std::this_thread::get_id();

        if (!this->threadID.has_value())
        {
            this->threadID = currentThreadID;
            return;
        }

        assert(this->threadID == currentThreadID);
#endif
    }
};

}  // namespace chatterino
