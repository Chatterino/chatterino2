#pragma once

#include <QList>
#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>

namespace chatterino {

template <class T>
class RatelimitBucket : public QObject
{
public:
    /**
     * @param budget            Capacity (and intitial amount) of tokens in the
     *                          bucket.
     * @param replenishInterval Time (in ms) it takes to replenish a single
     *                          token after it has been used. Set to 0 to
     *                          disable rate limiting.
     * @param callback          Function to apply to a bucket item when a token
     *                          is available.
     */
    RatelimitBucket(uint32_t budget, uint32_t replenishInterval,
                    std::function<void(const T &)> callback,
                    QObject *parent = nullptr)
        : QObject(parent)
        , budget_(budget)
        , unrestricted_(budget == 0)
        , replenishInterval_(replenishInterval)
        , callback_(callback)
        , queue_()
        , queueMutex_()
    {
    }

    // QList::append() only takes const T& anyway
    /**
     * Submit an item to the bucket. If a token is available, the item will be
     * handled immediately. Otherwise, handling is delayed until a token for
     * this item is replenished.
     */
    void submit(const T &argument)
    {
        if (this->unrestricted_)
        {
            // In "unrestricted mode", we are just a glorified proxy for the
            // callback.
            this->callback_(argument);
        }
        else
        {
            // In usual operation, we must check whether there is a token
            // available before handling.
            {
                std::lock_guard queueLock(this->queueMutex_);
                this->queue_.append(argument);
            }

            if (this->budget_ > 0)
            {
                this->handleOne();
            }
        }
    }

private:
    /// Denotes the amount of calls that can be handled before we need to wait
    /// for the cooldown.
    std::atomic<uint32_t> budget_;

    /// If `true`, rate limiting is disabled. Provided in order to allow for
    /// easy disabling of rate limits without having to change code at the call
    /// site.
    const bool unrestricted_;

    /// This is the amount of time in milliseconds it takes for one used up
    /// budget to be put back into the bucket for use elsewhere.
    const uint32_t replenishInterval_;

    std::function<void(const T &)> callback_;

    QList<T> queue_;
    std::mutex queueMutex_;

    /**
     * @brief Run the callback on one entry in the queue.
     *
     * This will start a timer that runs after cooldown_ milliseconds that
     * gives back one "token" to the bucket and calls handleOne again.
     **/
    void handleOne()
    {
        boost::optional<T> item;

        {
            std::lock_guard queueLock(this->queueMutex_);
            if (this->queue_.isEmpty())
            {
                return;
            }

            item = this->queue_.takeFirst();
        }

        this->budget_--;
        this->callback_(item.get());

        QTimer::singleShot(this->replenishInterval_, this, [this] {
            this->budget_++;
            this->handleOne();
        });
    }
};

}  // namespace chatterino
