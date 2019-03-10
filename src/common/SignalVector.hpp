#pragma once

#include <QStandardItemModel>
#include <QTimer>
#include <boost/noncopyable.hpp>
#include <pajlada/signals/signal.hpp>
#include <vector>

#include "util/AssertInGuiThread.hpp"

namespace chatterino
{
    template <typename TVectorItem>
    struct SignalVectorItemArgs
    {
        const TVectorItem& item;
        int index;
        void* caller;
    };

    template <typename TVectorItem>
    class ReadOnlySignalVector : boost::noncopyable
    {
    public:
        ReadOnlySignalVector()
        {
            QObject::connect(&this->itemsChangedTimer_, &QTimer::timeout,
                [this] { this->delayedItemsChanged.invoke(); });
            this->itemsChangedTimer_.setInterval(100);
            this->itemsChangedTimer_.setSingleShot(true);
        }
        virtual ~ReadOnlySignalVector() = default;

        pajlada::Signals::Signal<SignalVectorItemArgs<TVectorItem>>
            itemInserted;
        pajlada::Signals::Signal<SignalVectorItemArgs<TVectorItem>> itemRemoved;
        pajlada::Signals::NoArgSignal delayedItemsChanged;

        const std::vector<TVectorItem>& getVector() const
        {
            assertInGuiThread();

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
    };

    template <typename TVectorItem>
    class BaseSignalVector : public ReadOnlySignalVector<TVectorItem>
    {
    public:
        // returns the actual index of the inserted item
        virtual int insertItem(const TVectorItem& item, int proposedIndex = -1,
            void* caller = nullptr) = 0;

        void removeItem(int index, void* caller = nullptr)
        {
            assertInGuiThread();
            assert(index >= 0 && index < this->vector_.size());

            TVectorItem item = this->vector_[index];
            this->vector_.erase(this->vector_.begin() + index);
            SignalVectorItemArgs<TVectorItem> args{item, index, caller};
            this->itemRemoved.invoke(args);

            this->invokeDelayedItemsChanged();
        }

        int appendItem(const TVectorItem& item, void* caller = nullptr)
        {
            return this->insertItem(item, -1, caller);
        }
    };

    template <typename TVectorItem>
    class UnsortedSignalVector : public BaseSignalVector<TVectorItem>
    {
    public:
        virtual int insertItem(const TVectorItem& item, int index = -1,
            void* caller = nullptr) override
        {
            assertInGuiThread();
            if (index == -1)
            {
                index = this->vector_.size();
            }
            else
            {
                assert(index >= 0 && index <= this->vector_.size());
            }

            this->vector_.insert(this->vector_.begin() + index, item);

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
        virtual int insertItem(
            const TVectorItem& item, int = -1, void* caller = nullptr) override
        {
            assertInGuiThread();

            auto it = std::lower_bound(
                this->vector_.begin(), this->vector_.end(), item, Compare{});
            int index = it - this->vector_.begin();
            this->vector_.insert(it, item);

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
