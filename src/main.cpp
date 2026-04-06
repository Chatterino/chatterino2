// SPDX-FileCopyrightText: 2016 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "BrowserExtension.hpp"
#include "common/Args.hpp"
#include "common/Env.hpp"
#include "common/Modes.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "providers/IvrApi.hpp"
#include "providers/NetworkConfigurationProvider.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "RunGui.hpp"
#include "singletons/CrashHandler.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Updates.hpp"
#include "util/AttachToConsole.hpp"
#include "util/CombinePath.hpp"
#include "util/IpcQueue.hpp"

#ifdef Q_OS_MACOS
#    include "util/MacOsHelpers.h"
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSslSocket>
#include <QStringList>
#include <Qt>
#include <QtCore/QtPlugin>
#ifdef Q_OS_WIN
#    include <shobjidl_core.h>
#endif

#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

#include <cstring>
#include <memory>

#ifdef CHATTERINO_WITH_AVIF_PLUGIN
Q_IMPORT_PLUGIN(QAVIFPlugin)
#endif

using namespace chatterino;

namespace {

bool argvRequestsLegacyScaling(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] != nullptr &&
            std::strcmp(argv[i], "--use-old-scaling") == 0)
        {
            return true;
        }
    }
    return false;
}

bool jsonPointerBoolIsTrue(const QString &filePath, const char *jsonPointerPath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QByteArray data = f.readAll();
    rapidjson::Document doc;
    if (doc.Parse(data.constData(), data.size()).HasParseError())
    {
        return false;
    }
    const auto *v = rapidjson::Pointer(jsonPointerPath).Get(doc);
    return v != nullptr && v->IsBool() && v->GetBool();
}

// Find possible `settings.json` locations before QApplication is created
QStringList legacyScalingSettingsJsonCandidates(int argc, char **argv)
{
    QStringList candidates;
    const QFileInfo fi(QString::fromLocal8Bit(argc > 0 ? argv[0] : ""));
    const QString appDir = fi.absoluteDir().absolutePath();
    if (QFile::exists(combinePath(appDir, QStringLiteral("portable"))))
    {
        candidates << combinePath(appDir,
                                  QStringLiteral("Settings/settings.json"));
        return candidates;
    }

#if defined(Q_OS_WIN)
    QString base = QString::fromLocal8Bit(qgetenv("APPDATA"));
    if (!base.isEmpty())
    {
        base.replace(QStringLiteral("chatterino"), QStringLiteral("Chatterino"),
                     Qt::CaseInsensitive);
        base += QStringLiteral("2");
        candidates << combinePath(base,
                                  QStringLiteral("Settings/settings.json"));
    }
#elif defined(Q_OS_MACOS)
    const QString home = QString::fromLocal8Bit(qgetenv("HOME"));
    if (!home.isEmpty())
    {
        candidates << combinePath(
            combinePath(
                home, QStringLiteral("Library/Application Support/chatterino")),
            QStringLiteral("Settings/settings.json"));
    }
#else
    const QString home = QString::fromLocal8Bit(qgetenv("HOME"));
    if (!home.isEmpty())
    {
        QString xdgData = QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME"));
        if (xdgData.isEmpty())
        {
            xdgData = combinePath(home, QStringLiteral(".local/share"));
        }
        candidates << combinePath(
            combinePath(xdgData, QStringLiteral("chatterino")),
            QStringLiteral("Settings/settings.json"));
        candidates << combinePath(
            combinePath(xdgData, QStringLiteral("chatterino.com")),
            QStringLiteral("chatterino/Settings/settings.json"));
    }
#endif
    return candidates;
}

bool settingsFileRequestsLegacyScaling(int argc, char **argv)
{
    for (const auto &path : legacyScalingSettingsJsonCandidates(argc, argv))
    {
        if (jsonPointerBoolIsTrue(path, "/appearance/useLegacyScaling"))
        {
            return true;
        }
    }
    return false;
}

}  // namespace

int main(int argc, char **argv)
{
    if (argvRequestsLegacyScaling(argc, argv) ||
        settingsFileRequestsLegacyScaling(argc, argv))
    {
        QApplication::setAttribute(Qt::AA_Use96Dpi, true);
    }

    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("chatterino");
    QCoreApplication::setApplicationVersion(CHATTERINO_VERSION);
    QCoreApplication::setOrganizationDomain("chatterino.com");
#ifdef Q_OS_WIN
    SetCurrentProcessExplicitAppUserModelID(
        Version::instance().appUserModelID().c_str());
#endif

    std::unique_ptr<Paths> paths;

    try
    {
        paths = std::make_unique<Paths>();
    }
    catch (std::runtime_error &error)
    {
        QMessageBox box;
        if (Modes::instance().isPortable)
        {
            auto errorMessage =
                error.what() +
                QStringLiteral(
                    "\n\nInfo: Portable mode requires the application to "
                    "be in a writeable location. If you don't want "
                    "portable mode reinstall the application. "
                    "https://chatterino.com.");
            std::cerr << errorMessage.toLocal8Bit().constData() << '\n';
            std::cerr.flush();
            box.setText(errorMessage);
        }
        else
        {
            box.setText(error.what());
        }
        box.exec();
        return 1;
    }
    ipc::initPaths(paths.get());

    const Args args(a, *paths);

#ifdef CHATTERINO_WITH_CRASHPAD
    const auto crashpadHandler = installCrashHandler(args, *paths);
#endif

    // run in gui mode or browser extension host mode
    if (args.shouldRunBrowserExtensionHost)
    {
#ifdef Q_OS_MACOS
        ::chatterinoSetMacOsActivationPolicyProhibited();
#endif
        runBrowserExtensionHost();
    }
    else if (args.printVersion)
    {
        attachToConsole();

        auto version = Version::instance();
        auto versionMessage =
            QString("%1 (commit %2%3)")
                .arg(version.fullVersion())
                .arg(version.commitHash())
                .arg(version.isNightly() ? ", " + version.dateOfBuild() : "");
        std::cout << versionMessage.toLocal8Bit().constData() << '\n';
        std::cout.flush();
    }
    else
    {
        if (args.verbose)
        {
            attachToConsole();
        }

        qCInfo(chatterinoApp).noquote()
            << "Chatterino Qt SSL library build version:"
            << QSslSocket::sslLibraryBuildVersionString();
        qCInfo(chatterinoApp).noquote()
            << "Chatterino Qt SSL library version:"
            << QSslSocket::sslLibraryVersionString();
        qCInfo(chatterinoApp).noquote()
            << "Chatterino Qt SSL active backend:"
            << QSslSocket::activeBackend() << "of"
            << QSslSocket::availableBackends().join(", ");
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        qCInfo(chatterinoApp) << "Chatterino Qt SSL active backend features:"
                              << QSslSocket::supportedFeatures();
#endif
        qCInfo(chatterinoApp) << "Chatterino Qt SSL active backend protocols:"
                              << QSslSocket::supportedProtocols();

        Settings settings(args, paths->settingsDirectory);

        Updates updates(*paths, settings);

        NetworkConfigurationProvider::applyFromEnv(Env::get());

        IvrApi::initialize();
        Helix::initialize();

        runGui(a, *paths, settings, args, updates);
    }
    return 0;
}
