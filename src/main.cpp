#include "application.hpp"
#include "singletons/pathmanager.hpp"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QLibrary>

#include "util/networkmanager.hpp"

#ifdef USEWINSDK
#include "util/nativeeventhelper.hpp"
#endif

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_Use96Dpi, true);
#ifdef Q_OS_WIN32
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling, true);
#endif
    QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
    QApplication a(argc, argv);

// Install native event handler for hidpi on windows
#ifdef USEWINSDK
    a.installNativeEventFilter(new chatterino::util::DpiNativeEventFilter);
#endif

    // Initialize settings
    bool success = chatterino::singletons::PathManager::getInstance().init(argc, argv);
    if (!success) {
        printf("Error initializing paths\n");
        return 1;
    }

    // Initialize NetworkManager
    chatterino::util::NetworkManager::init();

    int ret = 0;

    {
        // Initialize application
        chatterino::Application app;

        // Start the application
        ret = app.run(a);

        // Application will go out of scope here and deinitialize itself
    }

    // Save settings
    pajlada::Settings::SettingManager::save();

    // Deinitialize NetworkManager (stop thread and wait for finish, should be instant)
    chatterino::util::NetworkManager::deinit();

    exit(0);
}
