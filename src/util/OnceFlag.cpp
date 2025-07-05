#include "util/OnceFlag.hpp"

namespace chatterino {

OnceFlag::OnceFlag() = default;

void OnceFlag::set()
{
    {
        std::unique_lock guard(this->mutex);
        this->flag.store(true, std::memory_order::relaxed);
    }
    this->condvar.notify_all();
}

bool OnceFlag::waitFor(std::chrono::milliseconds ms)
{
    std::unique_lock lock(this->mutex);
    if (this->flag.load(std::memory_order::relaxed))
    {
        return true;
    }
    return this->condvar.wait_for(lock, ms, [this] {
        return this->flag.load(std::memory_order::relaxed);
    });
}

void OnceFlag::wait()
{
    std::unique_lock lock(this->mutex);
    if (this->flag.load(std::memory_order::relaxed))
    {
        return;
    }
    this->condvar.wait(lock, [this] {
        return this->flag.load(std::memory_order::relaxed);
    });
}

bool OnceFlag::isSet()
{
    std::unique_lock lock(this->mutex);
    return this->flag.load(std::memory_order::relaxed);
}

}  // namespace chatterino
