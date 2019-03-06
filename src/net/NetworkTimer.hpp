#pragma once

#include <functional>

class QTimer;

namespace chatterino {

class NetworkWorker;

class NetworkTimer
{
    QTimer *timer_ = nullptr;

    bool started_{};

public:
    int timeoutMS_ = -1;

    NetworkTimer() = default;
    ~NetworkTimer() = default;

    NetworkTimer(const NetworkTimer &other) = delete;
    NetworkTimer &operator=(const NetworkTimer &other) = delete;

    NetworkTimer(NetworkTimer &&other) = default;
    NetworkTimer &operator=(NetworkTimer &&other) = default;

    void start();

    void onTimeout(NetworkWorker *worker, std::function<void()> cb) const;

    bool isStarted() const;
};

}  // namespace chatterino
