#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace chatterino {

/// @brief A flag that can only be set once which notifies waiters.
///
/// This can be used to synchronize with other threads. Note that waiting
/// threads will be suspended.
class OnceFlag
{
public:
    OnceFlag();
    ~OnceFlag();

    /// Set this flag and notify waiters
    void set();

    /// @brief Wait for at most `ms` until this flag is set.
    ///
    /// The calling thread will be suspended during the wait.
    ///
    /// @param ms The maximum time to wait for this flag
    /// @returns `true` if this flag was set during the wait or before
    bool waitFor(std::chrono::milliseconds ms);

    /// @brief Wait until this flag is set by another thread
    ///
    /// The calling thread will be suspended during the wait.
    void wait();

private:
    std::mutex mutex;
    std::condition_variable condvar;
    bool flag = false;
};

}  // namespace chatterino
