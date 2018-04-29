#pragma once

#include <QStandardItemModel>
#include <QTimer>
#include <boost/noncopyable.hpp>
#include <pajlada/signals/signal.hpp>
#include <vector>

#include "util/assertinguithread.hpp"

namespace chatterino {
namespace util {

template <typename TVectorItem>
class ReadOnlySignalVector : boost::noncopyable
{
public:
    ReadOnlySignalVector()
    {
        QObject::connect(&this->itemsChangedTimer, &QTimer::timeout,
                         [this] { this->delayedItemsChanged.invoke(); });
    }
    virtual ~ReadOnlySignalVector() = default;

    struct ItemArgs {
        const TVectorItem &item;
        int index;
        void *caller;
    };

    pajlada::Signals::Signal<ItemArgs> itemInserted;
    pajlada::Signals::Signal<ItemArgs> itemRemoved;
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
    virtual void appendItem(const TVectorItem &item, void *caller = 0) = 0;

    void removeItem(int index, void *caller = 0)
    {
        util::assertInGuiThread();
        assert(index >= 0 && index < this->vector.size());

        TVectorItem item = this->vector[index];
        this->vector.erase(this->vector.begin() + index);
        ItemArgs args{item, args, caller};
        this->itemRemoved.invoke(args);
    }
};

template <typename TVectorItem>
class UnsortedSignalVector : public BaseSignalVector<TVectorItem>
{
public:
    void insertItem(const TVectorItem &item, int index, void *caller = 0)
    {
        util::assertInGuiThread();
        assert(index >= 0 && index <= this->vector.size());

        this->vector.insert(this->vector.begin() + index, item);

        ItemArgs args{item, index, caller};
        this->itemInserted.invoke(args);
    }

    virtual void appendItem(const TVectorItem &item, void *caller = 0) override
    {
        this->insertItem(item, this->vector.size(), caller);
    }
};

template <typename TVectorItem>
class SortedSignalVector : public BaseSignalVector<TVectorItem>
{
public:
    virtual void appendItem(const TVectorItem &item, void *caller = 0) override
    {
        util::assertInGuiThread();

        int index = this->vector.insert(
                        std::lower_bound(this->vector.begin(), this->vector.end(), item), item) -
                    this->vector.begin();
        ItemArgs args{item, index, caller};
        this->itemInserted.invoke(args);
    }
};

}  // namespace util
}  // namespace chatterino
