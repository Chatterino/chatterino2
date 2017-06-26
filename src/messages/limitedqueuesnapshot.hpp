#pragma once

#include <memory>
#include <vector>

namespace chatterino {
namespace messages {

template <typename T>
class LimitedQueueSnapshot
{
public:
    LimitedQueueSnapshot(std::shared_ptr<std::vector<T>> _vector, std::size_t _offset,
                         std::size_t _size)
        : vector(_vector)
        , offset(_offset)
        , length(_size)
    {
    }

    std::size_t getLength()
    {
        return length;
    }

    T const &operator[](std::size_t index) const
    {
        return vector->at(index + offset);
    }

private:
    std::shared_ptr<std::vector<T>> vector;

    std::size_t offset;
    std::size_t length;
};

}  // namespace messages
}  // namespace chatterino
