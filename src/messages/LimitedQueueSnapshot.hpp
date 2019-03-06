#pragma once

#include <cassert>
#include <memory>
#include <vector>

namespace chatterino
{
    template <typename T>
    class LimitedQueueSnapshot
    {
    public:
        LimitedQueueSnapshot() = default;

        LimitedQueueSnapshot(
            std::shared_ptr<std::vector<std::shared_ptr<std::vector<T>>>>
                chunks,
            size_t length, size_t firstChunkOffset, size_t lastChunkEnd)
            : chunks_(chunks)
            , length_(length)
            , firstChunkOffset_(firstChunkOffset)
            , lastChunkEnd_(lastChunkEnd)
        {
        }

        std::size_t size()
        {
            return this->length_;
        }

        T const& operator[](std::size_t index) const
        {
            index += this->firstChunkOffset_;

            size_t x = 0;

            for (size_t i = 0; i < this->chunks_->size(); i++)
            {
                auto& chunk = this->chunks_->at(i);

                if (x <= index && x + chunk->size() > index)
                {
                    return chunk->at(index - x);
                }
                x += chunk->size();
            }

            assert(false && "out of range");

            return this->chunks_->at(0)->at(0);
        }

    private:
        std::shared_ptr<std::vector<std::shared_ptr<std::vector<T>>>> chunks_;

        size_t length_ = 0;
        size_t firstChunkOffset_ = 0;
        size_t lastChunkEnd_ = 0;
    };

}  // namespace chatterino
