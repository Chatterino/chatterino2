#include "util/IpcQueue.hpp"

#include "common/QLogging.hpp"

#include <boost/interprocess/ipc/message_queue.hpp>
#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace boost_ipc = boost::interprocess;

namespace chatterino::ipc {

void sendMessage(const char *name, const QByteArray &data)
{
    try
    {
        boost_ipc::message_queue messageQueue(boost_ipc::open_only, name);

        messageQueue.try_send(data.data(), size_t(data.size()), 1);
    }
    catch (boost_ipc::interprocess_exception &ex)
    {
        qCDebug(chatterinoNativeMessage)
            << "Failed to send message:" << ex.what();
    }
}

class IpcQueuePrivate
{
public:
    IpcQueuePrivate(const char *name, size_t maxMessages, size_t maxMessageSize)
        : queue(boost_ipc::open_or_create, name, maxMessages, maxMessageSize)
    {
    }

    boost_ipc::message_queue queue;
};

IpcQueue::IpcQueue() = default;
IpcQueue::~IpcQueue() = default;

std::optional<QString> IpcQueue::tryReplaceOrCreate(const char *name,
                                                    size_t maxMessages,
                                                    size_t maxMessageSize)
{
    try
    {
        Q_ASSERT_X(this->private_ == nullptr, "IpcQueue::tryReplaceOrCreate",
                   "The function can be called at most once.");

        boost_ipc::message_queue::remove(name);
        this->private_ = std::make_unique<IpcQueuePrivate>(name, maxMessages,
                                                           maxMessageSize);
        return std::nullopt;
    }
    catch (boost_ipc::interprocess_exception &ex)
    {
        return QString::fromLatin1(ex.what());
    }
}

QByteArray IpcQueue::receive()
{
    try
    {
        auto *d = this->private_.get();

        QByteArray buf;
        // The new storage is uninitialized
        buf.resize(static_cast<qsizetype>(d->queue.get_max_msg_size()));

        size_t messageSize = 0;
        unsigned int priority = 0;
        d->queue.receive(buf.data(), buf.size(), messageSize, priority);

        // truncate to the initialized storage
        buf.truncate(static_cast<qsizetype>(messageSize));
        return buf;
    }
    catch (boost_ipc::interprocess_exception &ex)
    {
        qCDebug(chatterinoNativeMessage)
            << "Failed to receive message:" << ex.what();
    }
    return {};
}

}  // namespace chatterino::ipc
