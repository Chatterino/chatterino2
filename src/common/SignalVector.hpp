#pragma once

#include "debug/AssertInGuiThread.hpp"

#include <pajlada/signals/signal.hpp>
#include <QStandardItemModel>
#include <QTimer>

#include <vector>

namespace chatterino {

template <typename T>
struct SignalVectorItemEvent {
    const T &item;
    int index;
    void *caller;
};

template <typename T>
class SignalVector
{
public:
    pajlada::Signals::Signal<SignalVectorItemEvent<T>> itemInserted;
    pajlada::Signals::Signal<SignalVectorItemEvent<T>> itemRemoved;
    pajlada::Signals::NoArgSignal delayedItemsChanged;

    SignalVector()
        : readOnly_(new std::vector<T>())
    {
        QObject::connect(&this->itemsChangedTimer_, &QTimer::timeout, [this] {
            this->delayedItemsChanged.invoke();
        });
        this->itemsChangedTimer_.setInterval(100);
        this->itemsChangedTimer_.setSingleShot(true);
    }

    SignalVector(std::function<bool(const T &, const T &)> &&compare)
        : SignalVector()
    {
        this->itemCompare_ = std::move(compare);
    }

    SignalVector(const SignalVector &) = delete;
    SignalVector &operator=(const SignalVector &) = delete;

    SignalVector(SignalVector &&) = delete;
    SignalVector &operator=(SignalVector &&) = delete;

    bool isSorted() const
    {
        return bool(this->itemCompare_);
    }

    /// A read-only version of the vector which can be used concurrently.
    std::shared_ptr<const std::vector<T>> readOnly()
    {
        return this->readOnly_;
    }

    /// This may only be called from the GUI thread.
    ///
    ///	@param item
    /// 	Item to be inserted.
    /// @param proposedIndex
    /// 	Index to insert at. `-1` will append at the end.
    ///		Will be ignored if the vector is sorted.
    /// @param caller
    ///     Caller id which will be passed in the itemInserted and itemRemoved
    /// 	signals.
    int insert(const T &item, int index = -1, void *caller = nullptr)
    {
        assertInGuiThread();

        if (this->isSorted())
        {
            auto it = std::lower_bound(this->items_.begin(), this->items_.end(),
                                       item, this->itemCompare_);
            index = it - this->items_.begin();
            this->items_.insert(it, item);
        }
        else
        {
            if (index == -1)
            {
                index = this->items_.size();
            }
            else
            {
                assert(index >= 0 &&
                       index <= static_cast<int>(this->items_.size()));
            }

            this->items_.insert(this->items_.begin() + index, item);
        }

        SignalVectorItemEvent<T> args{item, index, caller};
        this->itemInserted.invoke(args);
        this->itemsChanged_();

        return index;
    }

    /// This may only be called from the GUI thread.
    ///
    ///	@param item
    /// 	Item to be appended.
    /// @param caller
    ///     Caller id which will be passed in the itemInserted and itemRemoved
    /// 	signals.
    int append(const T &item, void *caller = nullptr)
    {
        assertInGuiThread();
        return this->insert(item, -1, caller);
    }

    void removeAt(int index, void *caller = nullptr)
    {
        assertInGuiThread();
        assert(index >= 0 && index < static_cast<int>(this->items_.size()));

        T item = this->items_[index];
        this->items_.erase(this->items_.begin() + index);

        SignalVectorItemEvent<T> args{item, index, caller};
        this->itemRemoved.invoke(args);

        this->itemsChanged_();
    }

    bool removeFirstMatching(std::function<bool(const T &)> matcher,
                             void *caller = nullptr)
    {
        assertInGuiThread();

        for (size_t index = 0; index < this->items_.size(); ++index)
        {
            T item = this->items_[index];
            if (matcher(item))
            {
                this->items_.erase(this->items_.begin() + index);
                SignalVectorItemEvent<T> args{item, static_cast<int>(index),
                                              caller};
                this->itemRemoved.invoke(args);
                this->itemsChanged_();
                return true;
            }
        }

        return false;
    }

    const std::vector<T> &raw() const
    {
        assertInGuiThread();

        return this->items_;
    }

    [[deprecated]] std::vector<T> cloneVector()
    {
        return *this->readOnly();
    }

    // mirror vector functions
    auto begin() const
    {
        assertInGuiThread();
        return this->items_.begin();
    }

    auto end() const
    {
        assertInGuiThread();
        return this->items_.end();
    }

    decltype(auto) operator[](size_t index)
    {
        assertInGuiThread();
        return this->items_[index];
    }

    auto empty()
    {
        assertInGuiThread();
        return this->items_.empty();
    }

private:
    void itemsChanged_()
    {
        // emit delayed event
        if (!this->itemsChangedTimer_.isActive())
        {
            this->itemsChangedTimer_.start();
        }

        // update concurrent version
        this->readOnly_ = std::make_shared<const std::vector<T>>(this->items_);
    }

    std::vector<T> items_;
    std::shared_ptr<const std::vector<T>> readOnly_;
    QTimer itemsChangedTimer_;
    std::function<bool(const T &, const T &)> itemCompare_;
};

}  // namespace chatterino
