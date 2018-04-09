#include "nativemessagingmanager.hpp"

#include "singletons/pathmanager.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

//#ifdef USEWINSDK
//#include <windows.h>
//#pragma comment(lib, "Advapi32.lib")
//#endif

#include <boost/interprocess/ipc/message_queue.hpp>

#ifdef Q_OS_WIN
#include <QProcess>
#endif

#define EXTENSION_ID "aeicjepmjkgmbeohnchmpfjbpchogmjn"
#define MESSAGE_SIZE 1024

namespace ipc = boost::interprocess;

namespace chatterino {
namespace singletons {

NativeMessagingManager::NativeMessagingManager()
{
}

NativeMessagingManager &NativeMessagingManager::getInstance()
{
    static NativeMessagingManager manager;
    return manager;
}

void NativeMessagingManager::registerHost()
{
    // create manifest
    QJsonDocument document;
    QJsonObject root_obj;
    root_obj.insert("name", "com.chatterino.chatterino");
    root_obj.insert("description", "Browser interaction with chatterino.");
    root_obj.insert("path", QCoreApplication::applicationFilePath());
    root_obj.insert("type", "stdio");

    QJsonArray allowed_origins_arr = {"chrome-extension://aeicjepmjkgmbeohnchmpfjbpchogmjn/"};
    root_obj.insert("allowed_origins", allowed_origins_arr);

    // save the manifest
    QString manifestPath =
        PathManager::getInstance().settingsFolderPath + "/native-messaging-manifest.json";

    document.setObject(root_obj);

    QFile file(manifestPath);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(document.toJson());
    file.flush();

#ifdef Q_OS_WIN
    // clang-format off
    QProcess::execute("REG ADD \"HKCU\\Software\\Google\\Chrome\\NativeMessagingHosts\\com.chatterino.chatterino\" /ve /t REG_SZ /d \"" + manifestPath + "\" /f");
// clang-format on
#endif
}

void NativeMessagingManager::openGuiMessageQueue()
{
    static ReceiverThread thread;

    if (thread.isRunning()) {
        thread.exit();
    }

    thread.start();
}

void NativeMessagingManager::sendToGuiProcess(const QByteArray &array)
{
    ipc::message_queue messageQueue(ipc::open_only, "chatterino_gui");

    try {
        messageQueue.try_send(array.data(), array.size(), 1);
    } catch (ipc::interprocess_exception &ex) {
        // rip
    }
}

void NativeMessagingManager::ReceiverThread::run()
{
    ipc::message_queue::remove("chatterino_gui");

    ipc::message_queue messageQueue(ipc::create_only, "chatterino_gui", 100, MESSAGE_SIZE);

    while (true) {
        try {
            char *buf = (char *)malloc(MESSAGE_SIZE);
            ipc::message_queue::size_type retSize;
            unsigned int priority;

            messageQueue.receive(buf, MESSAGE_SIZE, retSize, priority);

            QString text = QString::fromUtf8(buf, retSize);
            qDebug() << text;
        } catch (ipc::interprocess_exception &ex) {
            // rip
        }
    }
}
}  // namespace singletons
}  // namespace chatterino
