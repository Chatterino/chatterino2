#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QStringList>
#include <memory>

#include "BrowserExtension.hpp"
#include "RunGui.hpp"
#include "common/Args.hpp"
#include "common/Modes.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "providers/IvrApi.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/api/Kraken.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/IncognitoBrowser.hpp"

using namespace chatterino;

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("chatterino");
    QCoreApplication::setApplicationVersion(CHATTERINO_VERSION);
    QCoreApplication::setOrganizationDomain("https://www.chatterino.com");

    initArgs(a);

    // run in gui mode or browser extension host mode
    if (getArgs().shouldRunBrowserExtensionHost)
    {
        runBrowserExtensionHost();
    }
    else if (getArgs().printVersion)
    {
        auto version = Version::instance();
        qCInfo(chatterinoMain).noquote()
            << QString("%1 (commit %2%3)")
                   .arg(version.fullVersion())
                   .arg(version.commitHash())
                   .arg(Modes::instance().isNightly
                            ? ", " + version.dateOfBuild()
                            : "");
    }
    else
    {
        IvrApi::initialize();
        Helix::initialize();
        Kraken::initialize();

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

        Settings settings(paths->settingsDirectory);

        runGui(a, *paths, settings);
    }
    return 0;
}
