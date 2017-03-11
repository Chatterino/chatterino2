#include "channels.h"
#include "colorscheme.h"
#include "emojis.h"
#include "emotes.h"
#include "ircmanager.h"
#include "logging/loggingmanager.h"
#include "resources.h"
#include "settings.h"
#include "widgets/mainwindow.h"
#include "windows.h"

#include <QApplication>
#include <QClipboard>
#include <boost/signals2.hpp>

using namespace chatterino;
using namespace chatterino::widgets;

int
main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    chatterino::logging::init();
    chatterino::initChannels();
    Settings::getInstance().load();
    Resources::load();
    Emojis::loadEmojis();
    Emotes::loadGlobalEmotes();

    ColorScheme::getInstance().init();

    Windows::load();

    MainWindow &w = Windows::getMainWindow();
    w.show();

    IrcManager::connect();

    int ret = a.exec();

    Settings::getInstance().save();

    Windows::save();

    return ret;
}
