#include "singletons/NativeMessaging.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "util/IpcQueue.hpp"
#include "util/PostToThread.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#ifdef Q_OS_WIN
// clang-format off
#    include <QSettings>
// clang-format on
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
    QSettings registry(registryKeyName, QSettings::NativeFormat);
    registry.setValue("Default", manifestPath);
#endif
}

std::string &getNmQueueName(Paths &paths)
{
    static std::string name =
        "chatterino_gui" + paths.applicationFilePathHash.toStdString();
    return name;
}

// CLIENT

namespace nm_client {

    void sendMessage(const QByteArray &array)
    {
        ipc::sendMessage("chatterino_gui", array);
    }

    void writeToCout(const QByteArray &array)
    {
        const auto *data = array.data();
        auto size = uint32_t(array.size());

        // We're writing the raw bytes to cout.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        std::cout.write(reinterpret_cast<char *>(&size), 4);
        std::cout.write(data, size);
        std::cout.flush();
    }

}  // namespace nm_client

// SERVER

void NativeMessagingServer::start()
{
    this->thread.start();
}

void NativeMessagingServer::ReceiverThread::run()
{
    ipc::IpcQueue messageQueue;
    auto error =
        messageQueue.tryReplaceOrCreate("chatterino_gui", 100, MESSAGE_SIZE);
    if (error)
    {
        qCDebug(chatterinoNativeMessage)
            << "Failed to create message queue:" << *error;

        nmIpcError().set(*error);
        return;
    }

    while (true)
    {
        auto buf = messageQueue.receive();
        if (buf.isEmpty())
        {
            continue;
        }
        auto document = QJsonDocument::fromJson(buf);

        this->handleMessage(document.object());
    }
}

void NativeMessagingServer::ReceiverThread::handleMessage(
    const QJsonObject &root)
{
    QString action = root.value("action").toString();

    if (action.isNull())
    {
        qCDebug(chatterinoNativeMessage) << "NM action was null";
        return;
    }

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

        {
            const auto sizeObject = root.value("size").toObject();
            args.x = sizeObject.value("x").toDouble(-1.0);
            args.pixelRatio = sizeObject.value("pixelRatio").toDouble(-1.0);
            args.width = sizeObject.value("width").toInt(-1);
            args.height = sizeObject.value("height").toInt(-1);
        }

        args.fullscreen = attachFullscreen;

        qCDebug(chatterinoNativeMessage)
            << args.x << args.pixelRatio << args.width << args.height
            << args.winId;

        if (_type.isNull() || args.winId.isNull())
        {
            qCDebug(chatterinoNativeMessage)
                << "NM type, name or winId missing";
            attach = false;
            attachFullscreen = false;
            return;
        }
#endif

        if (_type == "twitch")
        {
            postToThread([=] {
                auto *app = getApp();

                if (!name.isEmpty())
                {
                    auto channel = app->twitch->getOrAddChannel(name);
                    if (app->twitch->watchingChannel.get() != channel)
                    {
                        app->twitch->watchingChannel.reset(channel);
                    }
                }

                if (attach || attachFullscreen)
                {
#ifdef USEWINSDK
                    auto *window = AttachedWindow::getForeground(args);
                    if (!name.isEmpty())
                    {
                        window->setChannel(app->twitch->getOrAddChannel(name));
                    }
#endif
                }
            });
        }
        else
        {
            qCDebug(chatterinoNativeMessage) << "NM unknown channel type";
        }
    }
    else if (action == "detach")
    {
        QString winId = root.value("winId").toString();

        if (winId.isNull())
        {
            qCDebug(chatterinoNativeMessage) << "NM winId missing";
            return;
        }

#ifdef USEWINSDK
        postToThread([winId] {
            qCDebug(chatterinoNativeMessage) << "NW detach";
            AttachedWindow::detach(winId);
        });
#endif
    }
    else
    {
        qCDebug(chatterinoNativeMessage) << "NM unknown action " + action;
    }
}

Atomic<boost::optional<QString>> &nmIpcError()
{
    static Atomic<boost::optional<QString>> x;
    return x;
}

}  // namespace chatterino
