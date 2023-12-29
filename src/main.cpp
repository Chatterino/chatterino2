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
#include "util/AttachToConsole.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QStringList>

#include <memory>

using namespace chatterino;

int main(int argc, char **argv)
{
    // TODO: This is a temporary fix (see #4552).
#if defined(Q_OS_WINDOWS) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
#endif

    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("chatterino");
    QCoreApplication::setApplicationVersion(CHATTERINO_VERSION);
    QCoreApplication::setOrganizationDomain("chatterino.com");

    Paths *paths{};

    try
    {
        paths = new Paths;
    }
    catch (std::runtime_error &error)
    {
        QMessageBox box;
        if (Modes::instance().isPortable)
        {
            box.setText(
                error.what() +
                QStringLiteral(
                    "\n\nInfo: Portable mode requires the application to "
                    "be in a writeable location. If you don't want "
                    "portable mode reinstall the application. "
                    "https://chatterino.com."));
        }
        else
        {
            box.setText(error.what());
        }
        box.exec();
        return 1;
    }

    const Args args(a);

#ifdef CHATTERINO_WITH_CRASHPAD
    const auto crashpadHandler = installCrashHandler(args);
#endif

    // run in gui mode or browser extension host mode
    if (args.shouldRunBrowserExtensionHost)
    {
        runBrowserExtensionHost();
    }
    else if (args.printVersion)
    {
        attachToConsole();

        auto version = Version::instance();
        qInfo().noquote() << QString("%1 (commit %2%3)")
                                 .arg(version.fullVersion())
                                 .arg(version.commitHash())
                                 .arg(Modes::instance().isNightly
                                          ? ", " + version.dateOfBuild()
                                          : "");
    }
    else
    {
        if (args.verbose)
        {
            attachToConsole();
        }

        NetworkConfigurationProvider::applyFromEnv(Env::get());

        IvrApi::initialize();
        Helix::initialize();

        Settings settings(paths->settingsDirectory);

        runGui(a, *paths, settings, args);
    }
    return 0;
}
