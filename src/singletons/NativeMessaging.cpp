#include "singletons/NativeMessaging.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "common/Modes.hpp"
#include "common/QLogging.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/IpcQueue.hpp"
#include "util/PostToThread.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QStringBuilder>

#ifdef Q_OS_WIN
#    include "widgets/AttachedWindow.hpp"
#endif

namespace {

using namespace chatterino;
using namespace chatterino::literals;

const QString EXTENSION_ID = u"glknmaideaikkmemifbfkhnomoknepka"_s;
constexpr const size_t MESSAGE_SIZE = 1024;

struct Config {
#ifdef Q_OS_WIN
    QString fileName;
    QString registryKey;
#else
    QString directory;
#endif
};

const Config FIREFOX{
#ifdef Q_OS_WIN
    .fileName = u"native-messaging-manifest-firefox.json"_s,
    .registryKey =
        u"HKCU\\Software\\Mozilla\\NativeMessagingHosts\\com.chatterino.chatterino"_s,
#elif defined(Q_OS_MACOS)
    .directory =
        u"~/Library/Application Support/Mozilla/NativeMessagingHosts"_s,
#else
    .directory = u"~/.mozilla/native-messaging-hosts"_s,
#endif
};

const Config CHROME{
#ifdef Q_OS_WIN
    .fileName = u"native-messaging-manifest-chrome.json"_s,
    .registryKey =
        u"HKCU\\Software\\Google\\Chrome\\NativeMessagingHosts\\com.chatterino.chatterino"_s,
#elif defined(Q_OS_MACOS)
    .directory =
        u"~/Library/Application Support/Google/Chrome/NativeMessagingHosts"_s,
#else
    .directory = u"~/.config/google-chrome/NativeMessagingHosts"_s,
#endif
};

void writeManifestTo(QString directory, const QString &filename,
                     const QJsonDocument &json)
{
    if (directory.startsWith('~'))
    {
        directory = QDir::homePath() % QStringView{directory}.sliced(1);
    }

    QDir dir(directory);
    if (!dir.mkpath(u"."_s))
    {
        qCWarning(chatterinoNativeMessage) << "Failed to create" << directory;
        return;
    }
    QFile file(dir.filePath(filename));
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCWarning(chatterinoNativeMessage)
            << "Failed to open" << filename << "in" << directory;
        return;
    }
    file.write(json.toJson());
}

void registerNmManifest([[maybe_unused]] const Paths &paths,
                        const Config &config, const QJsonDocument &document)
{
#ifdef Q_OS_WIN
    writeManifestTo(paths.miscDirectory, config.fileName, document);

    QSettings registry(config.registryKey, QSettings::NativeFormat);
    registry.setValue("Default",
                      QString(paths.miscDirectory % u'/' % config.fileName));
#else
    writeManifestTo(config.directory, u"com.chatterino.chatterino.json"_s,
                    document);
#endif
}

}  // namespace

namespace chatterino {

using namespace literals;

void registerNmHost(const Paths &paths)
{
    if (Modes::instance().isPortable)
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

    QStringList extensionIDs =
        getSettings()->additionalExtensionIDs.getValue().split(
            ';', Qt::SkipEmptyParts);

    // chrome
    {
        auto obj = getBaseDocument();
        QJsonArray allowedOriginsArr = {
            u"chrome-extension://%1/"_s.arg(EXTENSION_ID)};

        for (const auto &id : extensionIDs)
        {
            QString trimmedID = id.trimmed();
            if (!trimmedID.isEmpty())
            {
                allowedOriginsArr.append(
                    u"chrome-extension://%1/"_s.arg(trimmedID));
            }
        }

        obj.insert("allowed_origins", allowedOriginsArr);

        registerNmManifest(paths, CHROME, QJsonDocument{obj});
    }

    // firefox
    {
        auto obj = getBaseDocument();
        QJsonArray allowedExtensions = {"chatterino_native@chatterino.com"};

        for (const auto &id : extensionIDs)
        {
            QString trimmedID = id.trimmed();
            if (!trimmedID.isEmpty())
            {
                allowedExtensions.append(trimmedID);
            }
        }

        obj.insert("allowed_extensions", allowedExtensions);

        registerNmManifest(paths, FIREFOX, QJsonDocument{obj});
    }
}

std::string &getNmQueueName(const Paths &paths)
{
    static std::string name =
        "chatterino_gui" + paths.applicationFilePathHash.toStdString();
    return name;
}

// CLIENT

namespace nm::client {

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

}  // namespace nm::client

// SERVER
NativeMessagingServer::NativeMessagingServer()
    : thread(new ReceiverThread(*this))
{
    this->thread->setObjectName("C2NMReceiver");
}

NativeMessagingServer::~NativeMessagingServer()
{
    if (!ipc::IpcQueue::remove("chatterino_gui"))
    {
        qCWarning(chatterinoNativeMessage) << "Failed to remove message queue";
    }
    this->thread->requestInterruption();
    this->thread->quit();
    // Most likely, the receiver thread will still wait for a message
    if (!this->thread->wait(100))
    {
        this->thread->terminate();

        if (!this->thread->wait(100))
        {
            qCWarning(chatterinoNativeMessage)
                << "Failed to terminate thread cleanly";
        }
    }
}

void NativeMessagingServer::start()
{
    this->thread->start();
}

NativeMessagingServer::ReceiverThread::ReceiverThread(
    NativeMessagingServer &parent)
    : parent_(parent)
{
}

void NativeMessagingServer::ReceiverThread::run()
{
    auto [messageQueue, error] =
        ipc::IpcQueue::tryReplaceOrCreate("chatterino_gui", 100, MESSAGE_SIZE);

    if (!error.isEmpty())
    {
        qCDebug(chatterinoNativeMessage)
            << "Failed to create message queue:" << error;

        nmIpcError().set(error);
        return;
    }

    while (!this->isInterruptionRequested())
    {
        auto buf = messageQueue->receive();
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
    QString action = root["action"_L1].toString();

    if (action == "select")
    {
        this->handleSelect(root);
        return;
    }
    if (action == "detach")
    {
        this->handleDetach(root);
        return;
    }
    if (action == "sync")
    {
        this->handleSync(root);
        return;
    }

    qCDebug(chatterinoNativeMessage) << "NM unknown action" << action;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
void NativeMessagingServer::ReceiverThread::handleSelect(
    const QJsonObject &root)
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

    if (type != u"twitch"_s)
    {
        qCDebug(chatterinoNativeMessage) << "NM unknown channel type";
        return;
    }

    postToThread([=] {
        if (!name.isEmpty())
        {
            auto channel = getApp()->getTwitch()->getOrAddChannel(name);
            if (getApp()->getTwitch()->getWatchingChannel().get() != channel)
            {
                getApp()->getTwitch()->setWatchingChannel(channel);
            }
        }

        if (attach || attachFullscreen)
        {
#ifdef USEWINSDK
            auto *window = AttachedWindow::getForeground(args);
            if (!name.isEmpty())
            {
                window->setChannel(
                    getApp()->getTwitch()->getOrAddChannel(name));
            }
#endif
        }
    });
}

void NativeMessagingServer::ReceiverThread::handleDetach(
    const QJsonObject &root)
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
// NOLINTEND(readability-convert-member-functions-to-static)

void NativeMessagingServer::ReceiverThread::handleSync(const QJsonObject &root)
{
    // Structure:
    // { action: 'sync', twitchChannels?: string[] }
    postToThread([&parent = this->parent_,
                  twitch = root["twitchChannels"_L1].toArray()] {
        parent.syncChannels(twitch);
    });
}

void NativeMessagingServer::syncChannels(const QJsonArray &twitchChannels)
{
    assertInGuiThread();

    std::vector<ChannelPtr> updated;
    updated.reserve(twitchChannels.size());
    for (const auto &value : twitchChannels)
    {
        auto name = value.toString();
        if (name.isEmpty())
        {
            continue;
        }
        // the deduping is done on the extension side
        updated.emplace_back(getApp()->getTwitch()->getOrAddChannel(name));
    }

    // This will destroy channels that aren't used anymore.
    this->channelWarmer_ = std::move(updated);
}

Atomic<std::optional<QString>> &nmIpcError()
{
    static Atomic<std::optional<QString>> x;
    return x;
}

}  // namespace chatterino
