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

const QString CRASHPAD_EXECUTABLE_NAME = QStringLiteral("crashpad_handler.exe");

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

    const auto reportsDir =
        base::FilePath(nativeString(getPaths()->crashdumpDirectory));
    const auto metricsDir =
        base::FilePath(nativeString(getPaths()->crashMetricsDirectory));

    auto client = std::make_unique<crashpad::CrashpadClient>();

    if (!client->StartHandler(handlerPath, reportsDir, metricsDir, {}, {}, {},
                              true, false))
    {
        qCDebug(chatterinoApp) << "Failed to start crashpad handler";
        return nullptr;
    }

    qCDebug(chatterinoApp) << "Started crashpad handler";
    return client;
}

}  // namespace chatterino

#endif
