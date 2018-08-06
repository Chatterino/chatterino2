#pragma once

#include <QThread>

class Application;
class Paths;

namespace chatterino {

void registerNmHost(Application &app);
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
