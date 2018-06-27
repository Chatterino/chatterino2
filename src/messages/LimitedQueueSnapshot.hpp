#pragma once

#include <cassert>
#include <memory>
#include <vector>

namespace chatterino {

template <typename T>
class LimitedQueueSnapshot
{
public:
    LimitedQueueSnapshot() = default;

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

    size_t length = 0;
    size_t firstChunkOffset = 0;
    size_t lastChunkEnd = 0;
};

}  // namespace chatterino
