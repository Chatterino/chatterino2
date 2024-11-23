#include "util/OnceFlag.hpp"

namespace chatterino {

OnceFlag::OnceFlag() = default;
OnceFlag::~OnceFlag() = default;

void OnceFlag::set()
{
    {
        std::unique_lock guard(this->mutex);
        this->flag = true;
    }
    this->condvar.notify_all();
}

bool OnceFlag::waitFor(std::chrono::milliseconds ms)
{
    std::unique_lock lock(this->mutex);
    return this->condvar.wait_for(lock, ms, [this] {
        return this->flag;
    });
}

void OnceFlag::wait()
{
    std::unique_lock lock(this->mutex);
    this->condvar.wait(lock, [this] {
        return this->flag;
    });
}

}  // namespace chatterino
