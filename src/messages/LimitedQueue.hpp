#pragma once

#include "messages/LimitedQueueSnapshot.hpp"

#include <QDebug>

#include <memory>
#include <mutex>
#include <vector>

namespace chatterino {

//
// Warning:
// - this class is so overengineered it's not even funny anymore
//
// Explanation:
// - messages can be appended until 'limit' is reached
// - when the limit is reached for every message added one will be removed at
// the start
// - messages can only be added to the start when there is space for them,
//   trying to add messages to the start when it's full will not add them
// - you are able to get a "Snapshot" which captures the state of this object
// - adding items to this class does not change the "items" of the snapshot
//

template <typename T>
class LimitedQueue
{
protected:
    using Chunk = std::vector<T>;
    using ChunkVector = std::vector<std::shared_ptr<Chunk>>;

public:
    LimitedQueue(size_t limit = 1000)
        : limit_(limit)
    {
        this->clear();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        this->chunks_ = std::make_shared<ChunkVector>();
        auto chunk = std::make_shared<Chunk>();
        chunk->resize(this->chunkSize_);
        this->chunks_->push_back(chunk);
        this->firstChunkOffset_ = 0;
        this->lastChunkEnd_ = 0;
    }

    // return true if an item was deleted
    // deleted will be set if the item was deleted
    bool pushBack(const T &item, T &deleted)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        auto lastChunk = this->chunks_->back();

        if (lastChunk->size() <= this->lastChunkEnd_)
        {
            // Last chunk is full, create a new one and rebuild our chunk vector
            auto newVector = std::make_shared<ChunkVector>();

            // copy chunks
            for (auto &chunk : *this->chunks_)
            {
                newVector->push_back(chunk);
            }

            // push back new chunk
            auto newChunk = std::make_shared<Chunk>();
            newChunk->resize(this->chunkSize_);
            newVector->push_back(newChunk);

            // replace current chunk vector
            this->chunks_ = newVector;
            this->lastChunkEnd_ = 0;
            lastChunk = this->chunks_->back();
        }

        lastChunk->at(this->lastChunkEnd_++) = item;

        return this->deleteFirstItem(deleted);
    }

    // returns a vector with all the accepted items
    std::vector<T> pushFront(const std::vector<T> &items)
    {
        std::vector<T> acceptedItems;

        if (this->space() > 0)
        {
            std::lock_guard<std::mutex> lock(this->mutex_);

            // create new vector to clone chunks into
            auto newChunks = std::make_shared<ChunkVector>();

            newChunks->resize(this->chunks_->size());

            // copy chunks except for first one
            for (size_t i = 1; i < this->chunks_->size(); i++)
            {
                newChunks->at(i) = this->chunks_->at(i);
            }

            // create new chunk for the first one
            size_t offset =
                std::min(this->space(), static_cast<qsizetype>(items.size()));
            auto newFirstChunk = std::make_shared<Chunk>();
            newFirstChunk->resize(this->chunks_->front()->size() + offset);

            for (size_t i = 0; i < offset; i++)
            {
                newFirstChunk->at(i) = items[items.size() - offset + i];
                acceptedItems.push_back(items[items.size() - offset + i]);
            }

            for (size_t i = 0; i < this->chunks_->at(0)->size(); i++)
            {
                newFirstChunk->at(i + offset) = this->chunks_->at(0)->at(i);
            }

            newChunks->at(0) = newFirstChunk;

            this->chunks_ = newChunks;
            // qDebug() << acceptedItems.size();
            // qDebug() << this->chunks->at(0)->size();

            if (this->chunks_->size() == 1)
            {
                this->lastChunkEnd_ += offset;
            }
        }

        return acceptedItems;
    }

    // replace an single item, return index if successful, -1 if unsuccessful
    int replaceItem(const T &item, const T &replacement)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        int x = 0;

        for (size_t i = 0; i < this->chunks_->size(); i++)
        {
            auto &chunk = this->chunks_->at(i);

            size_t start = i == 0 ? this->firstChunkOffset_ : 0;
            size_t end =
                i == chunk->size() - 1 ? this->lastChunkEnd_ : chunk->size();

            for (size_t j = start; j < end; j++)
            {
                if (chunk->at(j) == item)
                {
                    auto newChunk = std::make_shared<Chunk>();
                    newChunk->resize(chunk->size());

                    for (size_t k = 0; k < chunk->size(); k++)
                    {
                        newChunk->at(k) = chunk->at(k);
                    }

                    newChunk->at(j) = replacement;
                    this->chunks_->at(i) = newChunk;

                    return x;
                }
                x++;
            }
        }

        return -1;
    }

    // replace an item at index, return true if worked
    bool replaceItem(size_t index, const T &replacement)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        size_t x = 0;

        for (size_t i = 0; i < this->chunks_->size(); i++)
        {
            auto &chunk = this->chunks_->at(i);

            size_t start = i == 0 ? this->firstChunkOffset_ : 0;
            size_t end =
                i == chunk->size() - 1 ? this->lastChunkEnd_ : chunk->size();

            for (size_t j = start; j < end; j++)
            {
                if (x == index)
                {
                    auto newChunk = std::make_shared<Chunk>();
                    newChunk->resize(chunk->size());

                    for (size_t k = 0; k < chunk->size(); k++)
                    {
                        newChunk->at(k) = chunk->at(k);
                    }

                    newChunk->at(j) = replacement;
                    this->chunks_->at(i) = newChunk;

                    return true;
                }
                x++;
            }
        }
        return false;
    }

    //    void insertAfter(const std::vector<T> &items, const T &index)

    LimitedQueueSnapshot<T> getSnapshot()
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        return LimitedQueueSnapshot<T>(
            this->chunks_, this->limit_ - this->space(),
            this->firstChunkOffset_, this->lastChunkEnd_);
    }

private:
    qsizetype space()
    {
        size_t totalSize = 0;
        for (auto &chunk : *this->chunks_)
        {
            totalSize += chunk->size();
        }

        totalSize -= this->chunks_->back()->size() - this->lastChunkEnd_;
        if (this->chunks_->size() != 1)
        {
            totalSize -= this->firstChunkOffset_;
        }

        return this->limit_ - totalSize;
    }

    bool deleteFirstItem(T &deleted)
    {
        // determine if the first chunk should be deleted
        if (space() > 0)
        {
            return false;
        }

        deleted = this->chunks_->front()->at(this->firstChunkOffset_);

        // need to delete the first chunk
        if (this->firstChunkOffset_ == this->chunks_->front()->size() - 1)
        {
            // copy the chunk vector
            auto newVector = std::make_shared<ChunkVector>();

            // delete first chunk
            bool first = true;
            for (auto &chunk : *this->chunks_)
            {
                if (!first)
                {
                    newVector->push_back(chunk);
                }
                first = false;
            }

            this->chunks_ = newVector;
            this->firstChunkOffset_ = 0;
        }
        else
        {
            this->firstChunkOffset_++;
        }

        return true;
    }

    std::shared_ptr<ChunkVector> chunks_;
    std::mutex mutex_;

    size_t firstChunkOffset_;
    size_t lastChunkEnd_;
    const size_t limit_;

    const size_t chunkSize_ = 100;
};

}  // namespace chatterino
