#pragma once

#include <QObject>
#include <memory>

namespace chatterino
{
    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    class MessageContainer : public QObject
    {
        Q_OBJECT

    public:
        struct Insertion
        {
            size_t index;
            std::vector<MessagePtr> messages;
        };

        struct Erasure
        {
            size_t index;
            size_t count;
        };

        explicit MessageContainer(QObject* parent = nullptr);

        const MessagePtr& operator[](size_t index) const;
        size_t size() const;
        std::vector<MessagePtr>::const_iterator begin() const;
        std::vector<MessagePtr>::const_iterator end() const;

        void insert(size_t index, const MessagePtr&);
        void insert(size_t index, const std::vector<MessagePtr>&);
        void append(const MessagePtr&);
        void append(const std::vector<MessagePtr>&);
        void remove(size_t index);
        void removeFromTo(size_t index, size_t end);
        void removeAmount(size_t index, size_t count);

    signals:
        void inserted(const Insertion& changes);
        void erased(const Erasure& changes);

    private:
        std::vector<MessagePtr> messages_;
    };
}  // namespace chatterino
