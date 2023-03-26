#ifdef CHATTERINO_WITH_CRASHPAD
#    include "providers/Crashpad.hpp"

#    include "common/QLogging.hpp"
#    include "singletons/Paths.hpp"

#    include <QApplication>
#    include <QDir>
#    include <QString>

#    include <memory>
#    include <string>

namespace {

/// The name of the crashpad handler executable.
/// This varies across platforms
#    if defined(Q_OS_UNIX)
const QString CRASHPAD_EXECUTABLE_NAME = QStringLiteral("crashpad_handler");
#    elif defined(Q_OS_WINDOWS)
const QString CRASHPAD_EXECUTABLE_NAME = QStringLiteral("crashpad_handler.exe");
#    else
#        error Unsupported platform
#    endif

/// Converts a QString into the platform string representation.
#    if defined(Q_OS_UNIX)
std::string nativeString(const QString &s)
{
    return s.toStdString();
}
#    elif defined(Q_OS_WINDOWS)
std::wstring nativeString(const QString &s)
{
    return s.toStdWString();
}
#    else
#        error Unsupported platform
#    endif

}  // namespace

namespace chatterino {

std::unique_ptr<crashpad::CrashpadClient> installCrashHandler()
{
    // Currently, the following directory layout is assumed:
    // [applicationDirPath]
    //  │
    //  ├─chatterino
    //  │
    //  ╰─[crashpad]
    //     │
    //     ╰─crashpad_handler
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

    const auto handlerPath = base::FilePath(nativeString(
        crashpadBinDir.absoluteFilePath(CRASHPAD_EXECUTABLE_NAME)));

    // Argument passed in --database
    // > Crash reports are written to this database, and if uploads are enabled,
    //   uploaded from this database to a crash report collection server.
    const auto databaseDir =
        base::FilePath(nativeString(getPaths()->crashdumpDirectory));

    auto client = std::make_unique<crashpad::CrashpadClient>();

    // See https://chromium.googlesource.com/crashpad/crashpad/+/HEAD/handler/crashpad_handler.md
    // for documentation on available options.
    if (!client->StartHandler(handlerPath, databaseDir, {}, {}, {}, {}, true,
                              false))
    {
        qCDebug(chatterinoApp) << "Failed to start crashpad handler";
        return nullptr;
    }

    qCDebug(chatterinoApp) << "Started crashpad handler";
    return client;
}

}  // namespace chatterino

#endif
