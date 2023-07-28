#pragma once

#include "common/Atomic.hpp"

#include <boost/optional.hpp>
#include <QString>
#include <QThread>

namespace chatterino {

class Application;
class Paths;

void registerNmHost(Paths &paths);
std::string &getNmQueueName(Paths &paths);

Atomic<boost::optional<QString>> &nmIpcError();

namespace nm_client {

    void sendMessage(const QByteArray &array);
    void writeToCout(const QByteArray &array);

}  // namespace nm_client

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
