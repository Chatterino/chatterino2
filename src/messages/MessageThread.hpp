#pragma once

#include <QString>

#include <memory>
#include <vector>

namespace chatterino {
struct Message;

class MessageThread
{
public:
    MessageThread(std::shared_ptr<const Message> rootMessage);
    ~MessageThread();

    void addToThread(const std::shared_ptr<const Message> &message);
    void addToThread(const std::weak_ptr<const Message> &message);

    /// Returns the number of live reply references
    size_t liveCount() const;

    /// Returns the number of live reply references
    size_t liveCount(const std::shared_ptr<const Message> &exclude) const;

    const QString &rootId() const
    {
        return rootMessageId_;
    }

    const std::shared_ptr<const Message> &root() const
    {
        return rootMessage_;
    }

    const std::vector<std::weak_ptr<const Message>> &replies() const
    {
        return replies_;
    }

private:
    const QString rootMessageId_;
    const std::shared_ptr<const Message> rootMessage_;
    std::vector<std::weak_ptr<const Message>> replies_;
};

}  // namespace chatterino
