#include "singletons/NativeMessaging.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Paths.hpp"
#include "util/PostToThread.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

namespace ipc = boost::interprocess;

#ifdef Q_OS_WIN
#    include <QProcess>

#    include <Windows.h>
#    include "singletons/WindowManager.hpp"
#    include "widgets/AttachedWindow.hpp"
#endif

#include <iostream>

#define EXTENSION_ID "glknmaideaikkmemifbfkhnomoknepka"
#define MESSAGE_SIZE 1024

namespace chatterino {

void registerNmManifest(Paths &paths, const QString &manifestFilename,
                        const QString &registryKeyName,
                        const QJsonDocument &document);

void registerNmHost(Paths &paths)
{
    if (paths.isPortable())
        return;

    auto getBaseDocument = [&] {
        QJsonObject obj;
        obj.insert("name", "com.chatterino.chatterino");
        obj.insert("description", "Browser interaction with chatterino.");
        obj.insert("path", QCoreApplication::applicationFilePath());
        obj.insert("type", "stdio");

        return obj;
    };

    // chrome
    {
        QJsonDocument document;

        auto obj = getBaseDocument();
        QJsonArray allowed_origins_arr = {"chrome-extension://" EXTENSION_ID
                                          "/"};
        obj.insert("allowed_origins", allowed_origins_arr);
        document.setObject(obj);

        registerNmManifest(paths, "/native-messaging-manifest-chrome.json",
                           "HKCU\\Software\\Google\\Chrome\\NativeMessagingHost"
                           "s\\com.chatterino.chatterino",
                           document);
    }

    // firefox
    {
        QJsonDocument document;

        auto obj = getBaseDocument();
        QJsonArray allowed_extensions = {"chatterino_native@chatterino.com"};
        obj.insert("allowed_extensions", allowed_extensions);
        document.setObject(obj);

        registerNmManifest(paths, "/native-messaging-manifest-firefox.json",
                           "HKCU\\Software\\Mozilla\\NativeMessagingHosts\\com."
                           "chatterino.chatterino",
                           document);
    }
}

void registerNmManifest(Paths &paths, const QString &manifestFilename,
                        const QString &registryKeyName,
                        const QJsonDocument &document)
{
    (void)registryKeyName;

    // save the manifest
    QString manifestPath = paths.miscDirectory + manifestFilename;
    QFile file(manifestPath);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(document.toJson());
    file.flush();

#ifdef Q_OS_WIN
    // clang-format off
        QProcess::execute("REG ADD \"" + registryKeyName + "\" /ve /t REG_SZ /d \"" + manifestPath + "\" /f");
// clang-format on
#endif
}

std::string &getNmQueueName(Paths &paths)
{
    static std::string name =
        "chatterino_gui" + paths.applicationFilePathHash.toStdString();
    return name;
}

// CLIENT

void NativeMessagingClient::sendMessage(const QByteArray &array)
{
    try
    {
        ipc::message_queue messageQueue(ipc::open_only, "chatterino_gui");

        messageQueue.try_send(array.data(), size_t(array.size()), 1);
        //        messageQueue.timed_send(array.data(), size_t(array.size()), 1,
        //                                boost::posix_time::second_clock::local_time() +
        //                                    boost::posix_time::seconds(10));
    }
    catch (ipc::interprocess_exception &ex)
    {
        qDebug() << "send to gui process:" << ex.what();
    }
}

void NativeMessagingClient::writeToCout(const QByteArray &array)
{
    auto *data = array.data();
    auto size = uint32_t(array.size());

    std::cout.write(reinterpret_cast<char *>(&size), 4);
    std::cout.write(data, size);
    std::cout.flush();
}

// SERVER

void NativeMessagingServer::start()
{
    this->thread.start();
}

void NativeMessagingServer::ReceiverThread::run()
{
    ipc::message_queue::remove("chatterino_gui");
    ipc::message_queue messageQueue(ipc::open_or_create, "chatterino_gui", 100,
                                    MESSAGE_SIZE);

    while (true)
    {
        try
        {
            auto buf = std::make_unique<char[]>(MESSAGE_SIZE);
            auto retSize = ipc::message_queue::size_type();
            auto priority = static_cast<unsigned int>(0);

            messageQueue.receive(buf.get(), MESSAGE_SIZE, retSize, priority);

            auto document = QJsonDocument::fromJson(
                QByteArray::fromRawData(buf.get(), retSize));

            this->handleMessage(document.object());
        }
        catch (ipc::interprocess_exception &ex)
        {
            qDebug() << "received from gui process:" << ex.what();
        }
    }
}

void NativeMessagingServer::ReceiverThread::handleMessage(
    const QJsonObject &root)
{
    auto app = getApp();

    QString action = root.value("action").toString();

    if (action.isNull())
    {
        qDebug() << "NM action was null";
        return;
    }

    qDebug() << root;

    if (action == "select")
    {
        QString _type = root.value("type").toString();
        bool attach = root.value("attach").toBool();
        bool attachFullscreen = root.value("attach_fullscreen").toBool();
        QString name = root.value("name").toString();

#ifdef USEWINSDK
        AttachedWindow::GetArgs args;
        args.winId = root.value("winId").toString();
        args.yOffset = root.value("yOffset").toInt(-1);
        args.width = root.value("size").toObject().value("width").toInt(-1);
        args.height = root.value("size").toObject().value("height").toInt(-1);
        args.fullscreen = attachFullscreen;

        qDebug() << args.width << args.height << args.winId;

        if (_type.isNull() || args.winId.isNull())
        {
            qDebug() << "NM type, name or winId missing";
            attach = false;
            attachFullscreen = false;
            return;
        }
#endif

        if (_type == "twitch")
        {
            postToThread([=] {
                if (!name.isEmpty())
                {
                    app->twitch.server->watchingChannel.reset(
                        app->twitch.server->getOrAddChannel(name));
                }

                if (attach || attachFullscreen)
                {
#ifdef USEWINSDK
                    //                    if (args.height != -1) {
                    auto *window =
                        AttachedWindow::get(::GetForegroundWindow(), args);
                    if (!name.isEmpty())
                    {
                        window->setChannel(
                            app->twitch.server->getOrAddChannel(name));
                    }
//                    }
//                    window->show();
#endif
                }
            });
        }
        else
        {
            qDebug() << "NM unknown channel type";
        }
    }
    else if (action == "detach")
    {
        QString winId = root.value("winId").toString();

        if (winId.isNull())
        {
            qDebug() << "NM winId missing";
            return;
        }

#ifdef USEWINSDK
        postToThread([winId] {
            qDebug() << "NW detach";
            AttachedWindow::detach(winId);
        });
#endif
    }
    else
    {
        qDebug() << "NM unknown action " + action;
    }
}

}  // namespace chatterino
