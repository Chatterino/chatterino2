#pragma once

#include <QStandardItemModel>
#include <pajlada/signals/signal.hpp>
#include <vector>

#include "util/assertinguithread.hpp"

namespace chatterino {
namespace util {

template <typename TVectorItem>
class ReadOnlySignalVector
{
public:
    virtual ~ReadOnlySignalVector() = default;

    struct ItemInsertedArgs {
        const TVectorItem &item;
        int index;
    };

    pajlada::Signals::Signal<ItemInsertedArgs> itemInserted;
    pajlada::Signals::Signal<int> itemRemoved;
    pajlada::Signals::NoArgSignal delayedItemsChanged;

    const std::vector<TVectorItem> &getVector() const
    {
        util::assertInGuiThread();

        return this->vector;
    }

protected:
    std::vector<TVectorItem> vector;
};

template <typename TVectorItem>
class BaseSignalVector : public ReadOnlySignalVector<TVectorItem>
{
public:
    void removeItem(int index)
    {
        util::assertInGuiThread();
        assert(index >= 0 && index < this->vector.size());

        this->vector.erase(this->vector.begin() + index);
        this->itemRemoved.invoke(index);
    }
};

template <typename TVectorItem>
class SignalVector2 : public BaseSignalVector<TVectorItem>
{
public:
    void insertItem(const TVectorItem &item, int index)
    {
        util::assertInGuiThread();
        assert(index >= 0 && index <= this->vector.size());

        this->vector.insert(this->vector.begin() + index, item);

        ItemInsertedArgs args{item, index};
        this->itemInserted.invoke(args);
    }

    void appendItem(const TVectorItem &item)
    {
        this->insertItem(item, this->vector.size());
    }
};

template <typename TVectorItem>
class SortedSignalVector : public BaseSignalVector<TVectorItem>
{
public:
    void addItem(const TVectorItem &item)
    {
        util::assertInGuiThread();

        int index = this->vector.insert(
                        std::lower_bound(this->vector.begin(), this->vector.end(), item), item) -
                    this->vector.begin();
        ItemInsertedArgs args{item, index};
        this->itemInserted.invoke(args);
    }
};

}  // namespace util
}  // namespace chatterino
