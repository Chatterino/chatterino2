#include "singletons/NativeMessaging.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "util/IpcQueue.hpp"
#include "util/PostToThread.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>

#ifdef Q_OS_WIN
#    include "widgets/AttachedWindow.hpp"
#endif

namespace {

using namespace chatterino;
using namespace literals;

const QString EXTENSION_ID = u"glknmaideaikkmemifbfkhnomoknepka"_s;
constexpr const size_t MESSAGE_SIZE = 1024;

void handleSelect(const QJsonObject &root)
{
    QString type = root["type"_L1].toString();
    bool attach = root["attach"_L1].toBool();
    bool attachFullscreen = root["attach_fullscreen"_L1].toBool();
    QString name = root["name"_L1].toString();

#ifdef USEWINSDK
    const auto sizeObject = root["size"_L1].toObject();
    AttachedWindow::GetArgs args = {
        .winId = root["winId"_L1].toString(),
        .yOffset = root["yOffset"_L1].toInt(-1),
        .x = sizeObject["x"_L1].toDouble(-1.0),
        .pixelRatio = sizeObject["pixelRatio"_L1].toDouble(-1.0),
        .width = sizeObject["width"_L1].toInt(-1),
        .height = sizeObject["height"_L1].toInt(-1),
        .fullscreen = attachFullscreen,
    };

    qCDebug(chatterinoNativeMessage)
        << args.x << args.pixelRatio << args.width << args.height << args.winId;

    if (args.winId.isNull())
    {
        qCDebug(chatterinoNativeMessage) << "winId in select is missing";
        return;
    }
#endif

    if (type != u"twtich"_s)
    {
        qCDebug(chatterinoNativeMessage) << "NM unknown channel type";
        return;
    }

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

void handleDetach(const QJsonObject &root)
{
    QString winId = root["winId"_L1].toString();

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

void handleMessage(const QJsonObject &root)
{
    QString action = root["action"_L1].toString();

    if (action == "select")
    {
        handleSelect(root);
        return;
    }
    if (action == "detach")
    {
        handleDetach(root);
        return;
    }

    qCDebug(chatterinoNativeMessage) << "NM unknown action" << action;
}

}  // namespace

namespace chatterino {

using namespace literals;

void registerNmManifest(Paths &paths, const QString &manifestFilename,
                        const QString &registryKeyName,
                        const QJsonDocument &document);

void registerNmHost(Paths &paths)
{
    if (paths.isPortable())
    {
        return;
    }

    auto getBaseDocument = [] {
        return QJsonObject{
            {u"name"_s, "com.chatterino.chatterino"_L1},
            {u"description"_s, "Browser interaction with chatterino."_L1},
            {u"path"_s, QCoreApplication::applicationFilePath()},
            {u"type"_s, "stdio"_L1},
        };
    };

    // chrome
    {
        auto obj = getBaseDocument();
        QJsonArray allowedOriginsArr = {
            u"chrome-extension://%1/"_s.arg(EXTENSION_ID)};
        obj.insert("allowed_origins", allowedOriginsArr);

        registerNmManifest(paths, "/native-messaging-manifest-chrome.json",
                           "HKCU\\Software\\Google\\Chrome\\NativeMessagingHost"
                           "s\\com.chatterino.chatterino",
                           QJsonDocument(obj));
    }

    // firefox
    {
        auto obj = getBaseDocument();
        QJsonArray allowedExtensions = {"chatterino_native@chatterino.com"};
        obj.insert("allowed_extensions", allowedExtensions);

        registerNmManifest(paths, "/native-messaging-manifest-firefox.json",
                           "HKCU\\Software\\Mozilla\\NativeMessagingHosts\\com."
                           "chatterino.chatterino",
                           QJsonDocument(obj));
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

        handleMessage(document.object());
    }
}

Atomic<boost::optional<QString>> &nmIpcError()
{
    static Atomic<boost::optional<QString>> x;
    return x;
}

}  // namespace chatterino
