#include "providers/Crashpad.hpp"

#include "singletons/Paths.hpp"
#include "common/QLogging.hpp"

#include <QApplication>
#include <QDir>
#include <QString>

#include <memory>

namespace {

const QString CRASHPAD_EXECUTABLE_NAME = QStringLiteral("crashpad_handler.exe");

}  // namespace

namespace chatterino::crasquish {

std::unique_ptr<crashpad::CrashpadClient> installCrashHandler()
{
    auto crashpadBinDir = QDir(QApplication::applicationDirPath());

    if (!crashpadBinDir.cd("crashpad"))
    {
        return nullptr;
    }
    if (!crashpadBinDir.exists(CRASHPAD_EXECUTABLE_NAME))
    {
        return nullptr;
    }

    const auto handlerPath =
        base::FilePath(crashpadBinDir.absoluteFilePath(CRASHPAD_EXECUTABLE_NAME)
                           .toStdWString());

    const auto reportsDir = base::FilePath(getPaths()->crashdumpDirectory.toStdWString());
    const auto metricsDir = base::FilePath(getPaths()->crashMetricsDirectory.toStdWString());

    auto client = std::make_unique<crashpad::CrashpadClient>();

    if (!client->StartHandler(handlerPath, reportsDir, metricsDir, {}, {}, {},
                              true, false))
    {
        qCDebug(chatterinoApp) << "Failed to start crashpad handler";
        return nullptr;
    }

    return client;
}

}  // namespace chatterino::crasquish
