#pragma once

#include <QStandardItemModel>
#include <QTimer>
#include <boost/noncopyable.hpp>
#include <pajlada/signals/signal.hpp>
#include <vector>

#include "debug/AssertInGuiThread.hpp"

namespace chatterino {
namespace util {

template <typename TVectorItem>
struct SignalVectorItemArgs {
    const TVectorItem &item;
    int index;
    void *caller;
};

template <typename TVectorItem>
class ReadOnlySignalVector : boost::noncopyable
{
public:
    ReadOnlySignalVector()
    {
        QObject::connect(&this->itemsChangedTimer, &QTimer::timeout,
                         [this] { this->delayedItemsChanged.invoke(); });
        this->itemsChangedTimer.setInterval(100);
        this->itemsChangedTimer.setSingleShot(true);
    }
    virtual ~ReadOnlySignalVector() = default;

    pajlada::Signals::Signal<SignalVectorItemArgs<TVectorItem>> itemInserted;
    pajlada::Signals::Signal<SignalVectorItemArgs<TVectorItem>> itemRemoved;
    pajlada::Signals::NoArgSignal delayedItemsChanged;

    const std::vector<TVectorItem> &getVector() const
    {
        util::assertInGuiThread();

        return this->vector;
    }

    void invokeDelayedItemsChanged()
    {
        util::assertInGuiThread();

        if (!this->itemsChangedTimer.isActive()) {
            itemsChangedTimer.start();
        }
    }

protected:
    std::vector<TVectorItem> vector;
    QTimer itemsChangedTimer;
};

template <typename TVectorItem>
class BaseSignalVector : public ReadOnlySignalVector<TVectorItem>
{
public:
    // returns the actual index of the inserted item
    virtual int insertItem(const TVectorItem &item, int proposedIndex = -1, void *caller = 0) = 0;

    void removeItem(int index, void *caller = 0)
    {
        util::assertInGuiThread();
        assert(index >= 0 && index < this->vector.size());

        TVectorItem item = this->vector[index];
        this->vector.erase(this->vector.begin() + index);
        SignalVectorItemArgs<TVectorItem> args{item, index, caller};
        this->itemRemoved.invoke(args);

        this->invokeDelayedItemsChanged();
    }

    int appendItem(const TVectorItem &item, void *caller = 0)
    {
        return this->insertItem(item, -1, caller);
    }
};

template <typename TVectorItem>
class UnsortedSignalVector : public BaseSignalVector<TVectorItem>
{
public:
    virtual int insertItem(const TVectorItem &item, int index = -1, void *caller = 0) override
    {
        util::assertInGuiThread();
        if (index == -1) {
            index = this->vector.size();
        } else {
            assert(index >= 0 && index <= this->vector.size());
        }

        this->vector.insert(this->vector.begin() + index, item);

        SignalVectorItemArgs<TVectorItem> args{item, index, caller};
        this->itemInserted.invoke(args);
        this->invokeDelayedItemsChanged();
        return index;
    }
};

template <typename TVectorItem, typename Compare>
class SortedSignalVector : public BaseSignalVector<TVectorItem>
{
public:
    virtual int insertItem(const TVectorItem &item, int = -1, void *caller = nullptr) override
    {
        util::assertInGuiThread();

        auto it = std::lower_bound(this->vector.begin(), this->vector.end(), item, Compare{});
        int index = it - this->vector.begin();
        this->vector.insert(it, item);

        SignalVectorItemArgs<TVectorItem> args{item, index, caller};
        this->itemInserted.invoke(args);
        this->invokeDelayedItemsChanged();
        return index;
    }
};

}  // namespace util
}  // namespace chatterino
