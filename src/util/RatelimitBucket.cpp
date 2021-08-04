#include "RatelimitBucket.hpp"

#include <QTimer>

namespace chatterino {

RatelimitBucket::RatelimitBucket(int limit, int cooldown,
                                 std::function<void(QString)> callback)
    : limit_(limit)
    , cooldown_(cooldown)
    , callback_(callback)
{
}

void RatelimitBucket::send(QString message)
{
    this->queue_.append(message);

    if (this->pending_ < this->limit_)
    {
        this->execute();
    }
}

void RatelimitBucket::execute()
{
    if (queue_.isEmpty())
    {
        return;
    }

    auto item = queue_.takeFirst();

    this->pending_++;
    callback_(item);

    QTimer::singleShot(cooldown_, [this] {
        this->pending_--;
        this->execute();
    });
}

}  // namespace chatterino
