#include "application.hpp"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QDir>
#include <QLibrary>
#include <QStandardPaths>
#include <pajlada/settings/settingmanager.hpp>

#include "util/networkmanager.hpp"

#ifdef USEWINSDK
#include "util/nativeeventhelper.hpp"
#endif

namespace {

inline bool initSettings(bool portable)
{
    QString settingsPath;
    if (portable) {
        settingsPath.append(QDir::currentPath());
    } else {
        // Get settings path
        settingsPath.append(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        if (settingsPath.isEmpty()) {
            printf("Error finding writable location for settings\n");
            return false;
        }
    }

    if (!QDir().mkpath(settingsPath)) {
        printf("Error creating directories for settings: %s\n", qPrintable(settingsPath));
        return false;
    }
    settingsPath.append("/settings.json");

    pajlada::Settings::SettingManager::load(qPrintable(settingsPath));

    return true;
}

}  // namespace

int main(int argc, char *argv[])
{
    //    QApplication::setAttribute(Qt::AA_Use96Dpi, true);
    QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
    QApplication a(argc, argv);

// Install native event handler for hidpi on windows
#ifdef USEWINSDK
    a.installNativeEventFilter(new chatterino::util::DpiNativeEventFilter);
#endif

    // Options
    bool portable = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "portable") == 0) {
            portable = true;
        }
    }

    // Initialize settings
    if (!initSettings(portable)) {
        printf("Error initializing settings\n");
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

    return ret;
}
