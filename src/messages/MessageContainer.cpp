#include "MessageContainer.hpp"

namespace chatterino
{
    MessageContainer::MessageContainer(QObject* parent)
        : QObject(parent)
    {
    }

    const MessagePtr& MessageContainer::operator[](size_t index) const
    {
        return this->messages_[index];
    }

    size_t MessageContainer::size() const
    {
        return this->messages_.size();
    }

    void MessageContainer::insert(size_t index, const MessagePtr& item)
    {
        this->messages_.insert(this->messages_.begin() + index, item);

        emit this->inserted({index, {item}});
    }

    void MessageContainer::insert(
        size_t index, const std::vector<MessagePtr>& items)
    {
        this->messages_.insert(
            this->messages_.begin() + index, items.begin(), items.end());

        emit this->inserted({index, items});
    }

    void MessageContainer::append(const MessagePtr& item)
    {
        auto index = this->messages_.size();
        this->messages_.push_back(item);

        emit this->inserted({index, {item}});
    }

    void MessageContainer::append(const std::vector<MessagePtr>& items)
    {
        auto index = this->messages_.size();
        for (auto&& item : items)
            this->messages_.push_back(item);

        emit this->inserted({index, items});
    }

    void MessageContainer::remove(size_t index)
    {
        assert(false);
    }

    void MessageContainer::removeFromTo(size_t index, size_t end)
    {
        assert(false);
    }

    void MessageContainer::removeAmount(size_t index, size_t count)
    {
        assert(false);
    }
}  // namespace chatterino
