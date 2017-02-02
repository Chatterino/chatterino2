#ifndef LIMITEDQUEUE_H
#define LIMITEDQUEUE_H

#include "messages/limitedqueuesnapshot.h"

#include <boost/signals2.hpp>
#include <memory>
#include <mutex>
#include <vector>

namespace chatterino {
namespace messages {

template <typename T>
class LimitedQueue
{
public:
    LimitedQueue(int limit = 10, int buffer = 5)
        : vector(new std::vector<T>(limit + buffer))
        , vectorPtr(this->vector)
        , mutex()
        , offset(0)
        , length(0)
        , limit(limit)
        , buffer(buffer)
    {
    }

    void
    clear()
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        this->vector = new std::vector<T>(this->limit + this->buffer);
        this->vectorPtr = std::shared_ptr<std::vector<T>>(this->vector);

        this->offset = 0;
        this->length = 0;
    }

    // return true if an item was deleted
    // deleted will be set if the item was deleted
    bool
    appendItem(const T &item, T &deleted)
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        if (this->length == this->limit) {
            // vector is full
            if (this->offset == this->buffer) {
                deleted = this->vector->at(this->offset);

                // create new vector
                auto *vector = new std::vector<T>(this->limit + this->buffer);

                for (int i = 0; i < this->limit; i++) {
                    vector->at(i) = this->vector->at(i + this->offset);
                }

                vector->at(limit - 1) = item;

                this->offset = 0;

                this->vector = vector;
                this->vectorPtr = std::shared_ptr<std::vector<T>>(vector);

                return true;
            } else {
                // append item and remove first
                deleted = this->vector->at(this->offset);

                this->vector->at(this->length + this->offset) = item;
                this->offset++;

                return true;
            }
        } else {
            // append item
            this->vector->at(this->length) = item;
            this->length++;

            return false;
        }
    }

    messages::LimitedQueueSnapshot<T>
    getSnapshot()
    {
        std::lock_guard<std::mutex> lock(mutex);

        return LimitedQueueSnapshot<T>(vectorPtr, offset, length);
    }

private:
    std::vector<T> *vector;
    std::shared_ptr<std::vector<T>> vectorPtr;

    std::mutex mutex;

    int offset;
    int length;
    int limit;
    int buffer;
};
}
}

#endif  // LIMITEDQUEUE_H
