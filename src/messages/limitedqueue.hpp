#pragma once

#include "messages/limitedqueuesnapshot.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace chatterino {
namespace messages {

template <typename T>
class LimitedQueue
{
public:
    LimitedQueue(int limit = 1000, int buffer = 250)
        : _offset(0)
        , _limit(limit)
        , _buffer(buffer)
    {
        _vector = std::make_shared<std::vector<T>>();
        _vector->reserve(_limit + _buffer);
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _vector = std::make_shared<std::vector<T>>();
        _vector->reserve(_limit + _buffer);

        _offset = 0;
    }

    // return true if an item was deleted
    // deleted will be set if the item was deleted
    bool appendItem(const T &item, T &deleted)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_vector->size() >= _limit) {
            // vector is full
            if (_offset == _buffer) {
                deleted = _vector->at(_offset);

                // create new vector
                auto newVector = std::make_shared<std::vector<T>>();
                newVector->reserve(_limit + _buffer);

                for (unsigned int i = 0; i < _limit; ++i) {
                    newVector->push_back(_vector->at(i + _offset));
                }
                newVector->push_back(item);

                _offset = 0;
                _vector = newVector;

                return true;
            } else {
                deleted = _vector->at(_offset);

                // append item and increment offset("deleting" first element)
                _vector->push_back(item);
                _offset++;

                return true;
            }
        } else {
            // append item
            _vector->push_back(item);

            return false;
        }
    }

    messages::LimitedQueueSnapshot<T> getSnapshot()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_vector->size() < _limit) {
            return LimitedQueueSnapshot<T>(_vector, _offset, _vector->size());
        } else {
            return LimitedQueueSnapshot<T>(_vector, _offset, _limit);
        }
    }

private:
    std::shared_ptr<std::vector<T>> _vector;
    std::mutex _mutex;

    unsigned int _offset;
    unsigned int _limit;
    unsigned int _buffer;
};

}  // namespace messages
}  // namespace chatterino
