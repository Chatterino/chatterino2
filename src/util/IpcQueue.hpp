#pragma once

#include <memory>
#include <variant>

class QByteArray;
class QString;

namespace chatterino::ipc {

void sendMessage(const char *name, const QByteArray &data);

class IpcQueuePrivate;
class IpcQueue
{
public:
    IpcQueue(IpcQueue &&other);
    ~IpcQueue();

    static std::variant<IpcQueue, QString> tryReplaceOrCreate(
        const char *name, size_t maxMessages, size_t maxMessageSize);

    // TODO: use std::expected
    /// Try to receive a message.
    /// In the case of an error, the buffer is empty.
    QByteArray receive();

private:
    IpcQueue(IpcQueuePrivate *priv);

    std::unique_ptr<IpcQueuePrivate> private_;

    friend class IpcQueuePrivate;
};

}  // namespace chatterino::ipc
