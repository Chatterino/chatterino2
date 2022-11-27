#pragma once

#include <boost/optional.hpp>
#include <common/Atomic.hpp>
#include <QString>
#include <QThread>

namespace chatterino {

class Application;
class Paths;

void registerNmHost(Paths &paths);
std::string &getNmQueueName(Paths &paths);

Atomic<boost::optional<QString>> &nmIpcError();

class NativeMessagingClient final
{
public:
    void sendMessage(const QByteArray &array);
    void writeToCout(const QByteArray &array);
};

class NativeMessagingServer final
{
public:
    void start();

private:
    class ReceiverThread : public QThread
    {
    public:
        void run() override;

    private:
        void handleMessage(const QJsonObject &root);
    };

    ReceiverThread thread;
};

}  // namespace chatterino
