#include "RatelimitBucket.hpp"

#include <QTimer>

namespace chatterino {

RatelimitBucket::RatelimitBucket(int limit, int cooldown,
                                 std::function<void(QString)> callback,
                                 QObject *parent)
    : QObject(parent)
    , limit_(limit)
    , cooldown_(cooldown)
    , callback_(callback)
{
}

void RatelimitBucket::send(QString message)
{
    this->queue_.append(message);

    if (this->pending_ < this->limit_)
    {
        this->handleOne();
    }
}

void RatelimitBucket::handleOne()
{
    if (queue_.isEmpty())
    {
        return;
    }

    auto item = queue_.takeFirst();

    this->pending_++;
    callback_(item);

    QTimer::singleShot(cooldown_, this, [this] {
        this->pending_--;
        this->handleOne();
    });
}

}  // namespace chatterino
