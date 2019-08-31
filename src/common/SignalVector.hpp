#pragma once

#include <QStandardItemModel>
#include <QTimer>
#include <boost/noncopyable.hpp>
#include <pajlada/signals/signal.hpp>
#include <shared_mutex>
#include <vector>

#include "debug/AssertInGuiThread.hpp"

namespace chatterino {

template <typename TVectorItem>
struct SignalVectorItemArgs {
    const TVectorItem &item;
    int index;
    void *caller;
};

template <typename TVectorItem>
class ReadOnlySignalVector : boost::noncopyable
{
    using VecIt = typename std::vector<TVectorItem>::iterator;

public:
    struct Iterator
        : public std::iterator<std::input_iterator_tag, TVectorItem> {
        Iterator(VecIt &&it, std::shared_mutex &mutex)
            : it_(std::move(it))
            , lock_(mutex)
            , mutex_(mutex)
        {
        }

        Iterator(const Iterator &other)
            : it_(other.it_)
            , lock_(other.mutex_)
            , mutex_(other.mutex_)
        {
        }

        Iterator &operator=(const Iterator &other)
        {
            this->lock_ = std::shared_lock(other.mutex_.get());
            this->mutex_ = other.mutex_;

            return *this;
        }

        TVectorItem &operator*()
        {
            return it_.operator*();
        }

        Iterator &operator++()
        {
            ++this->it_;
            return *this;
        }

        bool operator==(const Iterator &other)
        {
            return this->it_ == other.it_;
        }

        bool operator!=(const Iterator &other)
        {
            return this->it_ != other.it_;
        }

        auto operator-(const Iterator &other)
        {
            return this->it_ - other.it_;
        }

    private:
        VecIt it_;
        std::shared_lock<std::shared_mutex> lock_;
        std::reference_wrapper<std::shared_mutex> mutex_;
    };

    ReadOnlySignalVector()
    {
        QObject::connect(&this->itemsChangedTimer_, &QTimer::timeout,
                         [this] { this->delayedItemsChanged.invoke(); });
        this->itemsChangedTimer_.setInterval(100);
        this->itemsChangedTimer_.setSingleShot(true);
    }
    virtual ~ReadOnlySignalVector() = default;

    pajlada::Signals::Signal<SignalVectorItemArgs<TVectorItem>> itemInserted;
    pajlada::Signals::Signal<SignalVectorItemArgs<TVectorItem>> itemRemoved;
    pajlada::Signals::NoArgSignal delayedItemsChanged;

    Iterator begin() const
    {
        return Iterator(
            const_cast<std::vector<TVectorItem> &>(this->vector_).begin(),
            this->mutex_);
    }

    Iterator end() const
    {
        return Iterator(
            const_cast<std::vector<TVectorItem> &>(this->vector_).end(),
            this->mutex_);
    }

    bool empty() const
    {
        std::shared_lock lock(this->mutex_);

        return this->vector_.empty();
    }

    const std::vector<TVectorItem> &getVector() const
    {
        assertInGuiThread();

        return this->vector_;
    }

    std::vector<TVectorItem> cloneVector() const
    {
        std::shared_lock lock(this->mutex_);

        return this->vector_;
    }

    void invokeDelayedItemsChanged()
    {
        assertInGuiThread();

        if (!this->itemsChangedTimer_.isActive())
        {
            this->itemsChangedTimer_.start();
        }
    }

    virtual bool isSorted() const = 0;

protected:
    std::vector<TVectorItem> vector_;
    QTimer itemsChangedTimer_;
    mutable std::shared_mutex mutex_;
};

template <typename TVectorItem>
class BaseSignalVector : public ReadOnlySignalVector<TVectorItem>
{
public:
    // returns the actual index of the inserted item
    virtual int insertItem(const TVectorItem &item, int proposedIndex = -1,
                           void *caller = nullptr) = 0;

    void removeItem(int index, void *caller = nullptr)
    {
        assertInGuiThread();
        std::unique_lock lock(this->mutex_);

        assert(index >= 0 && index < int(this->vector_.size()));

        TVectorItem item = this->vector_[index];

        this->vector_.erase(this->vector_.begin() + index);
        lock.unlock();  // manual unlock

        SignalVectorItemArgs<TVectorItem> args{item, index, caller};
        this->itemRemoved.invoke(args);

        this->invokeDelayedItemsChanged();
    }

    int appendItem(const TVectorItem &item, void *caller = nullptr)
    {
        return this->insertItem(item, -1, caller);
    }
};

template <typename TVectorItem>
class UnsortedSignalVector : public BaseSignalVector<TVectorItem>
{
public:
    virtual int insertItem(const TVectorItem &item, int index = -1,
                           void *caller = nullptr) override
    {
        assertInGuiThread();

        {
            std::unique_lock lock(this->mutex_);
            if (index == -1)
            {
                index = this->vector_.size();
            }
            else
            {
                assert(index >= 0 && index <= this->vector_.size());
            }

            this->vector_.insert(this->vector_.begin() + index, item);
        }

        SignalVectorItemArgs<TVectorItem> args{item, index, caller};
        this->itemInserted.invoke(args);
        this->invokeDelayedItemsChanged();
        return index;
    }

    virtual bool isSorted() const override
    {
        return false;
    }
};

template <typename TVectorItem, typename Compare>
class SortedSignalVector : public BaseSignalVector<TVectorItem>
{
public:
    virtual int insertItem(const TVectorItem &item, int = -1,
                           void *caller = nullptr) override
    {
        assertInGuiThread();
        int index = -1;

        {
            std::unique_lock lock(this->mutex_);

            auto it = std::lower_bound(this->vector_.begin(),
                                       this->vector_.end(), item, Compare{});
            index = it - this->vector_.begin();
            this->vector_.insert(it, item);
        }

        SignalVectorItemArgs<TVectorItem> args{item, index, caller};
        this->itemInserted.invoke(args);
        this->invokeDelayedItemsChanged();
        return index;
    }

    virtual bool isSorted() const override
    {
        return true;
    }
};

}  // namespace chatterino
