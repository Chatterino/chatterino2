#include "singletons/Crashpad.hpp"

#include "common/Args.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"

#include <QDir>
#include <QFile>
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

const QString RECOVERY_FILE = u"chatterino-recovery.bin"_s;

/// The recovery options are saved outside the settings
/// to be able to read them without loading the settings.
///
/// The flags are saved in the `RECOVERY_FILE` in their binary representation.
std::optional<CrashRecovery::Flags> readFlags(const Paths &paths)
{
    using Flag = CrashRecovery::Flag;

    QFile file(QDir(paths.crashdumpDirectory).filePath(RECOVERY_FILE));
    if (!file.open(QFile::ReadOnly))
    {
        return std::nullopt;
    }

    auto line = file.readLine(64);

    static_assert(std::is_same_v<std::underlying_type_t<Flag>, qulonglong>);

    bool ok = false;
    auto value = static_cast<Flag>(line.toULongLong(&ok));
    if (!ok)
    {
        return std::nullopt;
    }

    return value;
}

bool canRestart(const Paths &paths)
{
#ifdef NDEBUG
    const auto &args = chatterino::getArgs();
    bool noBadArgs =
        !args.isFramelessEmbed && !args.shouldRunBrowserExtensionHost;

    return noBadArgs && readFlags(paths)
                            .value_or(CrashRecovery::Flag::None)
                            .has(CrashRecovery::Flag::DoCrashRecovery);
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

void CrashRecovery::initialize(Settings & /*settings*/, Paths &paths)
{
    auto flags = readFlags(paths);
    if (flags)
    {
        this->currentFlags_ = *flags;
    }
    else
    {
        // By default, we don't restart after a crash.
        this->updateFlags(Flag::None);
    }
}

CrashRecovery::Flags CrashRecovery::recoveryFlags() const
{
    return this->currentFlags_;
}

void CrashRecovery::updateFlags(CrashRecovery::Flags flags)
{
    this->currentFlags_ = flags;

    QFile file(QDir(getPaths()->crashdumpDirectory).filePath(RECOVERY_FILE));
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCWarning(chatterinoApp) << "Failed to open" << file.fileName();
        return;
    }

    static_assert(std::is_same_v<std::underlying_type_t<Flag>, qulonglong>);
    file.write(QByteArray::number(
        static_cast<qulonglong>(this->currentFlags_.value())));
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
        qCDebug(chatterinoApp) << "Cannot find crashpad directory";
        return nullptr;
    }
    if (!crashpadBinDir.exists(CRASHPAD_EXECUTABLE_NAME))
    {
        qCDebug(chatterinoApp) << "Cannot find crashpad handler executable";
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
        qCDebug(chatterinoApp) << "Failed to start crashpad handler";
        return nullptr;
    }

    qCDebug(chatterinoApp) << "Started crashpad handler";
    return client;
}
#endif

}  // namespace chatterino
