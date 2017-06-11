#pragma once

#include <memory>
#include <vector>

namespace chatterino {
namespace messages {

template <typename T>
class LimitedQueueSnapshot
{
public:
    LimitedQueueSnapshot(std::shared_ptr<std::vector<T>> vector, int offset, int size)
        : _vector(vector)
        , _offset(offset)
        , _length(size)
    {
    }

    int getSize()
    {
        return _length;
    }

    T const &operator[](int index) const
    {
        return _vector->at(index + _offset);
    }

private:
    std::shared_ptr<std::vector<T>> _vector;

    int _offset;
    int _length;
};

}  // namespace messages
}  // namespace chatterino
