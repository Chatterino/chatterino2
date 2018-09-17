#pragma once

#include <QThread>

namespace chatterino {

class Application;
class Paths;

void registerNmHost(Paths &paths);
std::string &getNmQueueName(Paths &paths);

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
