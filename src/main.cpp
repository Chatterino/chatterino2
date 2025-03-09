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
#include "util/IpcQueue.hpp"

#ifdef Q_OS_MACOS
#    include "util/MacOsHelpers.h"
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QSslSocket>
#include <QStringList>
#ifdef Q_OS_WIN
#    include <shobjidl_core.h>
#endif

#include <memory>

using namespace chatterino;

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("chatterino");
    QCoreApplication::setApplicationVersion(CHATTERINO_VERSION);
    QCoreApplication::setOrganizationDomain("chatterino.com");
#ifdef Q_OS_WIN
    SetCurrentProcessExplicitAppUserModelID(L"ChatterinoTeam.Chatterino");
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
                .arg(Modes::instance().isNightly ? ", " + version.dateOfBuild()
                                                 : "");
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
        qCInfo(chatterinoApp).noquote()
            << "Chatterino Qt SSL active backend:"
            << QSslSocket::activeBackend() << "of"
            << QSslSocket::availableBackends().join(", ");
#    if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        qCInfo(chatterinoApp) << "Chatterino Qt SSL active backend features:"
                              << QSslSocket::supportedFeatures();
#    endif
        qCInfo(chatterinoApp) << "Chatterino Qt SSL active backend protocols:"
                              << QSslSocket::supportedProtocols();
#endif

        Settings settings(args, paths->settingsDirectory);

        Updates updates(*paths, settings);

        NetworkConfigurationProvider::applyFromEnv(Env::get());

        IvrApi::initialize();
        Helix::initialize();

        runGui(a, *paths, settings, args, updates);
    }
    return 0;
}
