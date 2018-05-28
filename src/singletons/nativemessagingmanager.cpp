#include "nativemessagingmanager.hpp"

#include "application.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/pathmanager.hpp"
#include "util/posttothread.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <boost/interprocess/ipc/message_queue.hpp>

namespace ipc = boost::interprocess;

#ifdef Q_OS_WIN
#include <QProcess>

#include <Windows.h>
#include "singletons/windowmanager.hpp"
#include "widgets/attachedwindow.hpp"
#endif

#include <iostream>

#define EXTENSION_ID "aeicjepmjkgmbeohnchmpfjbpchogmjn"
#define MESSAGE_SIZE 1024

namespace chatterino {
namespace singletons {

NativeMessagingManager::NativeMessagingManager()
{
    qDebug() << "init NativeMessagingManager";
}

void NativeMessagingManager::writeByteArray(QByteArray a)
{
    char *data = a.data();
    uint32_t size;
    size = a.size();
    std::cout.write(reinterpret_cast<char *>(&size), 4);
    std::cout.write(data, a.size());
    std::cout.flush();
}

void NativeMessagingManager::registerHost()
{
    auto app = getApp();

    if (app->paths->isPortable()) {
        return;
    }

    // create manifest
    QJsonDocument document;
    QJsonObject root_obj;
    root_obj.insert("name", "com.chatterino.chatterino");
    root_obj.insert("description", "Browser interaction with chatterino.");
    root_obj.insert("path", QCoreApplication::applicationFilePath());
    root_obj.insert("type", "stdio");

    // chrome
    QJsonArray allowed_origins_arr = {"chrome-extension://aeicjepmjkgmbeohnchmpfjbpchogmjn/"};
    root_obj.insert("allowed_origins", allowed_origins_arr);

    // firefox
    QJsonArray allowed_extensions = {"585a153c7e1ac5463478f25f8f12220e9097e716@temporary-addon"};
    root_obj.insert("allowed_extensions", allowed_extensions);

    // save the manifest
    QString manifestPath = app->paths->settingsFolderPath + "/native-messaging-manifest.json";

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
    ipc::message_queue messageQueue(ipc::open_or_create, "chatterino_gui", 100, MESSAGE_SIZE);

    try {
        messageQueue.try_send(array.data(), array.size(), 1);
    } catch (ipc::interprocess_exception &ex) {
        qDebug() << "send to gui process:" << ex.what();
    }
}

void NativeMessagingManager::ReceiverThread::run()
{
    ipc::message_queue::remove("chatterino_gui");

    ipc::message_queue messageQueue(ipc::open_or_create, "chatterino_gui", 100, MESSAGE_SIZE);

    while (true) {
        try {
            std::unique_ptr<char> buf(static_cast<char *>(malloc(MESSAGE_SIZE)));
            ipc::message_queue::size_type retSize;
            unsigned int priority;

            messageQueue.receive(buf.get(), MESSAGE_SIZE, retSize, priority);

            QJsonDocument document =
                QJsonDocument::fromJson(QByteArray::fromRawData(buf.get(), retSize));

            this->handleMessage(document.object());
        } catch (ipc::interprocess_exception &ex) {
            qDebug() << "received from gui process:" << ex.what();
        }
    }
}

void NativeMessagingManager::ReceiverThread::handleMessage(const QJsonObject &root)
{
    auto app = getApp();

    QString action = root.value("action").toString();

    if (action.isNull()) {
        qDebug() << "NM action was null";
        return;
    }

    if (action == "select") {
        QString _type = root.value("type").toString();
        bool attach = root.value("attach").toBool();
        QString name = root.value("name").toString();

#ifdef USEWINSDK
        widgets::AttachedWindow::GetArgs args;
        args.winId = root.value("winId").toString();
        args.yOffset = root.value("yOffset").toInt(-1);
        args.width = root.value("size").toObject().value("width").toInt(-1);
        args.height = root.value("size").toObject().value("height").toInt(-1);

        if (_type.isNull() || args.winId.isNull()) {
            qDebug() << "NM type, name or winId missing";
            attach = false;
            return;
        }
#endif

        if (_type == "twitch") {
            util::postToThread([=] {
                if (!name.isEmpty()) {
                    app->twitch.server->watchingChannel.update(
                        app->twitch.server->getOrAddChannel(name));
                }

                if (attach) {
#ifdef USEWINSDK
                    auto *window = widgets::AttachedWindow::get(::GetForegroundWindow(), args);
                    if (!name.isEmpty()) {
                        window->setChannel(app->twitch.server->getOrAddChannel(name));
                    }
//                    window->show();
#endif
                }
            });

        } else {
            qDebug() << "NM unknown channel type";
        }
    } else if (action == "detach") {
        QString winId = root.value("winId").toString();

        if (winId.isNull()) {
            qDebug() << "NM winId missing";
            return;
        }

#ifdef USEWINSDK
        util::postToThread([winId] { widgets::AttachedWindow::detach(winId); });
#endif
    } else {
        qDebug() << "NM unknown action " + action;
    }
}  // namespace singletons

}  // namespace singletons
}  // namespace chatterino
