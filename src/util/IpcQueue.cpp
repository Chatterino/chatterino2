#include "util/IpcQueue.hpp"

#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"

#define BOOST_INTERPROCESS_SHARED_DIR_FUNC
#include <boost/interprocess/ipc/message_queue.hpp>
#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace boost_ipc = boost::interprocess;

namespace {

static const chatterino::Paths *PATHS = nullptr;

}  // namespace

namespace boost::interprocess::ipcdetail {

void get_shared_dir(std::string &shared_dir)
{
    if (!PATHS)
    {
        assert(false && "PATHS not set");
        qCCritical(chatterinoNativeMessage)
            << "PATHS not set for shared directory";
        return;
    }
    shared_dir = PATHS->ipcDirectory.toStdString();
}

#ifdef BOOST_INTERPROCESS_WINDOWS
void get_shared_dir(std::wstring &shared_dir)
{
    if (!PATHS)
    {
        assert(false && "PATHS not set");
        qCCritical(chatterinoNativeMessage)
            << "PATHS not set for shared directory";
        return;
    }
    shared_dir = PATHS->ipcDirectory.toStdWString();
}
#endif

}  // namespace boost::interprocess::ipcdetail

namespace chatterino::ipc {

void initPaths(const Paths *paths)
{
    PATHS = paths;
}

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

IpcQueue::IpcQueue(IpcQueuePrivate *priv)
    : private_(priv){};
IpcQueue::~IpcQueue() = default;

std::pair<std::unique_ptr<IpcQueue>, QString> IpcQueue::tryReplaceOrCreate(
    const char *name, size_t maxMessages, size_t maxMessageSize)
{
    try
    {
        boost_ipc::message_queue::remove(name);
        return std::make_pair(
            std::unique_ptr<IpcQueue>(new IpcQueue(
                new IpcQueuePrivate(name, maxMessages, maxMessageSize))),
            QString());
    }
    catch (boost_ipc::interprocess_exception &ex)
    {
        return {nullptr, QString::fromLatin1(ex.what())};
    }
}

bool IpcQueue::remove(const char *name)
{
    return boost_ipc::message_queue::remove(name);
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
