#include "common/NetworkTimer.hpp"

#include "common/NetworkWorker.hpp"

#include <QTimer>

#include <cassert>

namespace chatterino
{
    void NetworkTimer::start()
    {
        if (this->timeoutMS_ <= 0)
        {
            return;
        }

        this->timer_ = new QTimer;
        this->timer_->start(this->timeoutMS_);
        QObject::connect(
            this->timer_, &QTimer::timeout, [timer = this->timer_] {
                timer->deleteLater();  //
            });

        this->started_ = true;
    }

    bool NetworkTimer::isStarted() const
    {
        return this->started_;
    }

    void NetworkTimer::onTimeout(
        NetworkWorker* worker, std::function<void()> cb) const
    {
        assert(this->timer_ != nullptr);
        assert(worker != nullptr);

        QObject::connect(this->timer_, &QTimer::timeout, worker, cb);
    }

}  // namespace chatterino
