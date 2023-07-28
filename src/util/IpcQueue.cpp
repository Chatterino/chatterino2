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
    IpcQueuePrivate(IpcQueue *q_ptr, const char *name, size_t maxMessages,
                    size_t maxMessageSize)
        : q_ptr(q_ptr)
        , queue_(boost_ipc::open_or_create, name, maxMessages, maxMessageSize)
    {
    }

    Q_DECLARE_PUBLIC(IpcQueue)
    IpcQueue *q_ptr;

    boost_ipc::message_queue queue_;
};

IpcQueue::IpcQueue() = default;
IpcQueue::~IpcQueue() = default;

std::optional<QString> IpcQueue::tryReplaceOrCreate(const char *name,
                                                    size_t maxMessages,
                                                    size_t maxMessageSize)
{
    try
    {
        Q_ASSERT_X(this->d_ptr.isNull(), "IpcQueue::tryReplaceOrCreate",
                   "The function can be called at most once.");

        boost_ipc::message_queue::remove(name);
        this->d_ptr.reset(
            new IpcQueuePrivate(this, name, maxMessages, maxMessageSize));
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
        Q_D(IpcQueue);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        QByteArray buf(static_cast<qsizetype>(d->queue_.get_max_msg_size()),
                       Qt::Uninitialized);
#else
        QByteArray buf;
        buf.resize(static_cast<qsizetype>(d->queue_.get_max_msg_size()));
#endif
        size_t messageSize = 0;
        unsigned int priority = 0;
        d->queue_.receive(buf.data(), buf.size(), messageSize, priority);

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
