#pragma once

#include "messages/limitedqueuesnapshot.hpp"

#include <QDebug>

#include <memory>
#include <mutex>
#include <vector>

namespace chatterino {
namespace messages {

//
// Warning:
// - this class is so overengineered it's not even funny anymore
//
// Explanation:
// - messages can be appended until 'limit' is reached
// - when the limit is reached for every message added one will be removed at the start
// - messages can only be added to the start when there is space for them,
//   trying to add messages to the start when it's full will not add them
// - you are able to get a "Snapshot" which captures the state of this object
// - adding items to this class does not change the "items" of the snapshot
//

template <typename T>
class LimitedQueue
{
protected:
    typedef std::shared_ptr<std::vector<T>> Chunk;
    typedef std::shared_ptr<std::vector<Chunk>> ChunkVector;

public:
    LimitedQueue(int _limit = 1000)
        : limit(_limit)
    {
        this->clear();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        this->chunks = std::make_shared<std::vector<std::shared_ptr<std::vector<T>>>>();
        Chunk chunk = std::make_shared<std::vector<T>>();
        chunk->resize(this->chunkSize);
        this->chunks->push_back(chunk);
        this->firstChunkOffset = 0;
        this->lastChunkEnd = 0;
    }

    // return true if an item was deleted
    // deleted will be set if the item was deleted
    bool pushBack(const T &item, T &deleted)
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        Chunk lastChunk = this->chunks->back();

        // still space in the last chunk
        if (lastChunk->size() <= this->lastChunkEnd) {
            // create new chunk vector
            ChunkVector newVector =
                std::make_shared<std::vector<std::shared_ptr<std::vector<T>>>>();

            // copy chunks
            for (Chunk &chunk : *this->chunks) {
                newVector->push_back(chunk);
            }

            // push back new chunk
            Chunk newChunk = std::make_shared<std::vector<T>>();
            newChunk->resize(this->chunkSize);
            newVector->push_back(newChunk);

            // replace current chunk vector
            this->chunks = newVector;
            this->lastChunkEnd = 0;
            lastChunk = this->chunks->back();
        }

        lastChunk->at(this->lastChunkEnd++) = item;

        return this->deleteFirstItem(deleted);
    }

    // returns a vector with all the accepted items
    std::vector<T> pushFront(const std::vector<T> &items)
    {
        std::vector<T> acceptedItems;

        if (this->space() > 0) {
            std::lock_guard<std::mutex> lock(this->mutex);

            // create new vector to clone chunks into
            ChunkVector newChunks =
                std::make_shared<std::vector<std::shared_ptr<std::vector<T>>>>();

            newChunks->resize(this->chunks->size());

            // copy chunks except for first one
            for (size_t i = 1; i < this->chunks->size(); i++) {
                newChunks->at(i) = this->chunks->at(i);
            }

            // create new chunk for the first one
            size_t offset = std::min(this->space(), items.size());
            Chunk newFirstChunk = std::make_shared<std::vector<T>>();
            newFirstChunk->resize(this->chunks->front()->size() + offset);

            for (size_t i = 0; i < offset; i++) {
                newFirstChunk->at(i) = items[items.size() - offset + i];
                acceptedItems.push_back(items[items.size() - offset + i]);
            }

            for (size_t i = 0; i < this->chunks->at(0)->size(); i++) {
                newFirstChunk->at(i + offset) = this->chunks->at(0)->at(i);
            }

            newChunks->at(0) = newFirstChunk;

            this->chunks = newChunks;
            // qDebug() << acceptedItems.size();
            // qDebug() << this->chunks->at(0)->size();

            if (this->chunks->size() == 1) {
                this->lastChunkEnd += offset;
            }
        }

        return acceptedItems;
    }

    // replace an single item, return index if successful, -1 if unsuccessful
    int replaceItem(const T &item, const T &replacement)
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        int x = 0;

        for (size_t i = 0; i < this->chunks->size(); i++) {
            Chunk &chunk = this->chunks->at(i);

            size_t start = i == 0 ? this->firstChunkOffset : 0;
            size_t end = i == chunk->size() - 1 ? this->lastChunkEnd : chunk->size();

            for (size_t j = start; j < end; j++) {
                if (chunk->at(j) == item) {
                    Chunk newChunk = std::make_shared<std::vector<T>>();
                    newChunk->resize(chunk->size());

                    for (size_t k = 0; k < chunk->size(); k++) {
                        newChunk->at(k) = chunk->at(k);
                    }

                    newChunk->at(j) = replacement;
                    this->chunks->at(i) = newChunk;

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
        std::lock_guard<std::mutex> lock(this->mutex);

        size_t x = 0;

        for (size_t i = 0; i < this->chunks->size(); i++) {
            Chunk &chunk = this->chunks->at(i);

            size_t start = i == 0 ? this->firstChunkOffset : 0;
            size_t end = i == chunk->size() - 1 ? this->lastChunkEnd : chunk->size();

            for (size_t j = start; j < end; j++) {
                if (x == index) {
                    Chunk newChunk = std::make_shared<std::vector<T>>();
                    newChunk->resize(chunk->size());

                    for (size_t k = 0; k < chunk->size(); k++) {
                        newChunk->at(k) = chunk->at(k);
                    }

                    newChunk->at(j) = replacement;
                    this->chunks->at(i) = newChunk;

                    return true;
                }
                x++;
            }
        }
        return false;
    }

    //    void insertAfter(const std::vector<T> &items, const T &index)

    messages::LimitedQueueSnapshot<T> getSnapshot()
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        return LimitedQueueSnapshot<T>(this->chunks, this->limit - this->space(),
                                       this->firstChunkOffset, this->lastChunkEnd);
    }

private:
    size_t space()
    {
        size_t totalSize = 0;
        for (Chunk &chunk : *this->chunks) {
            totalSize += chunk->size();
        }

        totalSize -= this->chunks->back()->size() - this->lastChunkEnd;
        if (this->chunks->size() != 1) {
            totalSize -= this->firstChunkOffset;
        }

        return this->limit - totalSize;
    }

    bool deleteFirstItem(T &deleted)
    {
        // determine if the first chunk should be deleted
        if (space() > 0) {
            return false;
        }

        deleted = this->chunks->front()->at(this->firstChunkOffset);

        this->firstChunkOffset++;

        // need to delete the first chunk
        if (this->firstChunkOffset == this->chunks->front()->size() - 1) {
            // copy the chunk vector
            ChunkVector newVector =
                std::make_shared<std::vector<std::shared_ptr<std::vector<T>>>>();

            // delete first chunk
            bool first = true;
            for (Chunk &chunk : *this->chunks) {
                if (!first) {
                    newVector->push_back(chunk);
                }
                first = false;
            }

            this->chunks = newVector;
            this->firstChunkOffset = 0;
        }
        return true;
    }

    ChunkVector chunks;
    std::mutex mutex;

    size_t firstChunkOffset;
    size_t lastChunkEnd;
    size_t limit;

    const size_t chunkSize = 100;
};

}  // namespace messages
}  // namespace chatterino
