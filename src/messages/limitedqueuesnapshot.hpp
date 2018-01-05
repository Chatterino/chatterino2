#pragma once

#include <memory>
#include <vector>

namespace chatterino {
namespace messages {

template <typename T>
class LimitedQueueSnapshot
{
public:
    LimitedQueueSnapshot()
        : length(0)
    {
    }

    LimitedQueueSnapshot(std::shared_ptr<std::vector<std::shared_ptr<std::vector<T>>>> _chunks,
                         size_t _length, size_t _firstChunkOffset, size_t _lastChunkEnd)
        : chunks(_chunks)
        , length(_length)
        , firstChunkOffset(_firstChunkOffset)
        , lastChunkEnd(_lastChunkEnd)
    {
    }

    std::size_t getLength()
    {
        return this->length;
    }

    T const &operator[](std::size_t index) const
    {
        index += this->firstChunkOffset;

        size_t x = 0;

        for (size_t i = 0; i < this->chunks->size(); i++) {
            auto &chunk = this->chunks->at(i);

            if (x <= index && x + chunk->size() > index) {
                return chunk->at(index - x);
            }
            x += chunk->size();
        }

        assert(false && "out of range");

        return this->chunks->at(0)->at(0);
    }

private:
    std::shared_ptr<std::vector<std::shared_ptr<std::vector<T>>>> chunks;

    size_t length;
    size_t firstChunkOffset;
    size_t lastChunkEnd;
};

}  // namespace messages
}  // namespace chatterino
