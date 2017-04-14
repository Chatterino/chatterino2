#ifndef LIMITEDQUEUE_H
#define LIMITEDQUEUE_H

#include "messages/limitedqueuesnapshot.h"

#include <memory>
#include <mutex>
#include <vector>

namespace  chatterino {
namespace  messages {

template <typename T>
class LimitedQueue
{
public:
    LimitedQueue(int limit = 100, int buffer = 25)
        : mutex()
        , offset(0)
        , limit(limit)
        , buffer(buffer)
    {
        vector = std::make_shared<std::vector<T>>();
        vector->reserve(this->limit + this->buffer);
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        this->vector = std::make_shared<std::vector<T>>();
        this->vector->reserve(this->limit + this->buffer);

        this->offset = 0;
    }

    // return true if an item was deleted
    // deleted will be set if the item was deleted
    bool appendItem(const T &item, T &deleted)
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        if (this->vector->size() >= this->limit) {
            // vector is full
            if (this->offset == this->buffer) {
                deleted = this->vector->at(this->offset);

                // create new vector
                auto newVector = std::make_shared<std::vector<T>>();
                newVector->reserve(this->limit + this->buffer);

                for (unsigned int i = 0; i < this->limit - 1; i++) {
                    newVector->push_back(this->vector->at(i + this->offset));
                }
                newVector->push_back(item);

                this->offset = 0;
                this->vector = newVector;

                return true;
            } else {
                deleted = this->vector->at(this->offset);

                // append item and increment offset("deleting" first element)
                this->vector->push_back(item);
                this->offset++;

                return true;
            }
        } else {
            // append item
            this->vector->push_back(item);

            return false;
        }
    }

    messages::LimitedQueueSnapshot<T> getSnapshot()
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        if (vector->size() < limit) {
            return LimitedQueueSnapshot<T>(this->vector, this->offset, this->vector->size());
        } else {
            return LimitedQueueSnapshot<T>(this->vector, this->offset, this->limit);
        }
    }

private:
    std::shared_ptr<std::vector<T>> vector;

    std::mutex mutex;

    unsigned int offset;
    unsigned int limit;
    unsigned int buffer;
};

}  // namespace  messages
}  // namespace  chatterino

#endif  // LIMITEDQUEUE_H
