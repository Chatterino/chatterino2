#include "singletons/CrashHandler.hpp"

#include "common/Args.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#ifdef CHATTERINO_WITH_CRASHPAD
#    include <QApplication>

#    include <memory>
#    include <string>
#endif

namespace {

using namespace chatterino;
using namespace literals;

/// The name of the crashpad handler executable.
/// This varies across platforms
#if defined(Q_OS_UNIX)
const QString CRASHPAD_EXECUTABLE_NAME = QStringLiteral("crashpad_handler");
#elif defined(Q_OS_WINDOWS)
const QString CRASHPAD_EXECUTABLE_NAME = QStringLiteral("crashpad_handler.exe");
#else
#    error Unsupported platform
#endif

/// Converts a QString into the platform string representation.
#if defined(Q_OS_UNIX)
std::string nativeString(const QString &s)
{
    return s.toStdString();
}
#elif defined(Q_OS_WINDOWS)
std::wstring nativeString(const QString &s)
{
    return s.toStdWString();
}
#else
#    error Unsupported platform
#endif

const QString RECOVERY_FILE = u"chatterino-recovery.json"_s;

/// The recovery options are saved outside the settings
/// to be able to read them without loading the settings.
///
/// The flags are saved in the `RECOVERY_FILE` as JSON.
std::optional<bool> readRecoverySettings(const Paths &paths)
{
    QFile file(QDir(paths.crashdumpDirectory).filePath(RECOVERY_FILE));
    if (!file.open(QFile::ReadOnly))
    {
        return std::nullopt;
    }

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        qCWarning(chatterinoCrashhandler)
            << "Failed to parse recovery settings" << error.errorString();
        return std::nullopt;
    }

    const auto obj = doc.object();
    auto shouldRecover = obj["shouldRecover"_L1];
    if (!shouldRecover.isBool())
    {
        return std::nullopt;
    }

    return shouldRecover.toBool();
}

bool canRestart(const Paths &paths)
{
#ifdef NDEBUG
    const auto &args = chatterino::getArgs();
    bool noBadArgs =
        !args.isFramelessEmbed && !args.shouldRunBrowserExtensionHost;

    return noBadArgs && readRecoverySettings(paths).value_or(false);
#else
    (void)paths;
    return false;
#endif
}

std::string encodeArguments()
{
    std::string args;
    for (auto arg : getArgs().currentArguments())
    {
        if (!args.empty())
        {
            args.push_back('+');
        }
        args += arg.replace(u'+', u"++"_s).toStdString();
    }
    return args;
}

}  // namespace

namespace chatterino {

using namespace std::string_literals;

void CrashHandler::initialize(Settings & /*settings*/, Paths &paths)
{
    auto optSettings = readRecoverySettings(paths);
    if (optSettings)
    {
        this->shouldRecover_ = *optSettings;
    }
    else
    {
        // By default, we don't restart after a crash.
        this->setShouldRecover(false);
    }
}

void CrashHandler::setShouldRecover(bool value)
{
    this->shouldRecover_ = value;

    QFile file(QDir(getPaths()->crashdumpDirectory).filePath(RECOVERY_FILE));
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCWarning(chatterinoCrashhandler)
            << "Failed to open" << file.fileName();
        return;
    }
    file.write(QJsonDocument(QJsonObject{
                                 {"shouldRecover"_L1, value},
                             })
                   .toJson(QJsonDocument::Compact));
}

#ifdef CHATTERINO_WITH_CRASHPAD
std::unique_ptr<crashpad::CrashpadClient> installCrashHandler()
{
    // Currently, the following directory layout is assumed:
    // [applicationDirPath]
    //  ├─chatterino(.exe)
    //  ╰─[crashpad]
    //     ╰─crashpad_handler(.exe)
    // TODO: The location of the binary might vary across platforms
    auto crashpadBinDir = QDir(QApplication::applicationDirPath());

    if (!crashpadBinDir.cd("crashpad"))
    {
        qCDebug(chatterinoCrashhandler) << "Cannot find crashpad directory";
        return nullptr;
    }
    if (!crashpadBinDir.exists(CRASHPAD_EXECUTABLE_NAME))
    {
        qCDebug(chatterinoCrashhandler)
            << "Cannot find crashpad handler executable";
        return nullptr;
    }

    auto handlerPath = base::FilePath(nativeString(
        crashpadBinDir.absoluteFilePath(CRASHPAD_EXECUTABLE_NAME)));

    // Argument passed in --database
    // > Crash reports are written to this database, and if uploads are enabled,
    //   uploaded from this database to a crash report collection server.
    auto databaseDir =
        base::FilePath(nativeString(getPaths()->crashdumpDirectory));

    auto client = std::make_unique<crashpad::CrashpadClient>();

    std::map<std::string, std::string> annotations{
        {
            "canRestart"s,
            canRestart(*getPaths()) ? "true"s : "false"s,
        },
        {
            "exePath"s,
            QApplication::applicationFilePath().toStdString(),
        },
        {
            "startedAt"s,
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString(),
        },
        {
            "exeArguments"s,
            encodeArguments(),
        },
    };

    // See https://chromium.googlesource.com/crashpad/crashpad/+/HEAD/handler/crashpad_handler.md
    // for documentation on available options.
    if (!client->StartHandler(handlerPath, databaseDir, {}, {}, {}, annotations,
                              {}, true, false))
    {
        qCDebug(chatterinoCrashhandler) << "Failed to start crashpad handler";
        return nullptr;
    }

    qCDebug(chatterinoCrashhandler) << "Started crashpad handler";
    return client;
}
#endif

}  // namespace chatterino
