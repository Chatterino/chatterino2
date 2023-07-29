#pragma once

#include <memory>
#include <optional>

class QByteArray;
class QString;

namespace chatterino::ipc {

void sendMessage(const char *name, const QByteArray &data);

class IpcQueuePrivate;
class IpcQueue
{
public:
    IpcQueue();
    ~IpcQueue();

    /// This must only be called at most once and before any call to `receive`.
    std::optional<QString> tryReplaceOrCreate(const char *name,
                                              size_t maxMessages,
                                              size_t maxMessageSize);

    // TODO: use std::expected
    /// Try to receive a message.
    /// `tryReplaceOrCreate` must have been called before.
    /// In the case of an error, the buffer is empty.
    QByteArray receive();

private:
    std::unique_ptr<IpcQueuePrivate> private_;

    friend class IpcQueuePrivate;
};

}  // namespace chatterino::ipc
