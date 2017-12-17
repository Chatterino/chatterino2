#include "application.hpp"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QDir>
#include <QLibrary>
#include <QStandardPaths>
#include <pajlada/settings/settingmanager.hpp>

#include "util/networkmanager.hpp"

#ifdef USEWINSDK
#include "windows.h"
#endif

namespace {

#ifdef USEWINSDK
class DpiNativeEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override
    {
        MSG *msg = reinterpret_cast<MSG *>(message);

        if (msg->message == WM_NCCREATE) {
            QLibrary user32("user32.dll", NULL);
            {
                typedef BOOL(WINAPI * EnableNonClientDpiScaling)(HWND);

                EnableNonClientDpiScaling enableNonClientDpiScaling =
                    (EnableNonClientDpiScaling)user32.resolve("EnableNonClientDpiScaling");

                if (enableNonClientDpiScaling)
                    enableNonClientDpiScaling(msg->hwnd);
            }
        }
        return false;
    }
};
#endif

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

#ifdef USEWINSDK
    a.installNativeEventFilter(new DpiNativeEventFilter);
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
