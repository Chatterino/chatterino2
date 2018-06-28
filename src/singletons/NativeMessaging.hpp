#pragma once

#include <QThread>

namespace chatterino {

class NativeMessaging
{
public:
    // fourtf: don't add this class to the application class
    NativeMessaging();

    ~NativeMessaging() = delete;

    class ReceiverThread : public QThread
    {
    public:
        void run() override;

    private:
        void handleMessage(const QJsonObject &root);
    };

    void writeByteArray(QByteArray a);
    void registerHost();
    void openGuiMessageQueue();
    void sendToGuiProcess(const QByteArray &array);

    static std::string &getGuiMessageQueueName();
};

}  // namespace chatterino
