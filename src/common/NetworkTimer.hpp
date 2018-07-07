#pragma once

#include "common/NetworkWorker.hpp"

#include <QTimer>

#include <cassert>
#include <functional>
#include <memory>

namespace chatterino {

class NetworkTimer
{
    std::unique_ptr<QTimer> timer_;

    bool started_{};

public:
    int timeoutMS_ = -1;

    NetworkTimer() = default;
    ~NetworkTimer()
    {
        this->timer_.release();
    }

    NetworkTimer(const NetworkTimer &other) = delete;
    NetworkTimer &operator=(const NetworkTimer &other) = delete;

    NetworkTimer(NetworkTimer &&other) = default;
    NetworkTimer &operator=(NetworkTimer &&other) = default;

    void start()
    {
        if (this->timeoutMS_ <= 0) {
            return;
        }

        this->timer_ = std::make_unique<QTimer>();
        this->timer_->start(this->timeoutMS_);

        this->started_ = true;
    }

    bool isStarted() const
    {
        return this->started_;
    }

    void onTimeout(NetworkWorker *worker, std::function<void()> cb) const
    {
        if (!this->timer_) {
            return;
        }

        QObject::connect(this->timer_.get(), &QTimer::timeout, worker, cb);
    }
};

}  // namespace chatterino
