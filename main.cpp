#include "channelmanager.h"
#include "colorscheme.h"
#include "common.h"
#include "emojis.h"
#include "emotemanager.h"
#include "ircmanager.h"
#include "logging/loggingmanager.h"
#include "resources.h"
#include "settingsmanager.h"
#include "widgets/mainwindow.h"
#include "windowmanager.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QStandardPaths>
#include <boost/signals2.hpp>
#include <pajlada/settings/settingmanager.hpp>

using namespace chatterino;
using namespace chatterino::widgets;

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
    QApplication a(argc, argv);

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

    chatterino::logging::init();
    SettingsManager::getInstance().load();
    Resources::load();
    Emojis::loadEmojis();
    EmoteManager::getInstance().loadGlobalEmotes();

    ColorScheme::getInstance().init();

    WindowManager::getInstance().load();

    MainWindow &w = WindowManager::getInstance().getMainWindow();
    w.show();

    IrcManager::getInstance().connect();

    int ret = a.exec();

    SettingsManager::getInstance().save();

    // Save settings
    pajlada::Settings::SettingManager::save();

    WindowManager::getInstance().save();

    return ret;
}
