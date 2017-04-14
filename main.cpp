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

using namespace  chatterino;
using namespace  chatterino::widgets;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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

    WindowManager::getInstance().save();

    return ret;
}
