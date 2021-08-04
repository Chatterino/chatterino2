#include "RatelimitBucket.hpp"

#include <QTimer>

namespace chatterino {

RatelimitBucket::RatelimitBucket(int limit, int cooldown,
                                 std::function<void(QString)> callback,
                                 QObject *parent)
    : QObject(parent)
    , budget_(limit)
    , cooldown_(cooldown)
    , callback_(callback)
{
}

void RatelimitBucket::send(QString message)
{
    this->queue_.append(message);

    if (this->budget_ > 0)
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

    this->budget_--;
    callback_(item);

    QTimer::singleShot(cooldown_, this, [this] {
        this->budget_++;
        this->handleOne();
    });
}

}  // namespace chatterino
