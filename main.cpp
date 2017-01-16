#include "channels.h"
#include "colorscheme.h"
#include "emojis.h"
#include "ircmanager.h"
#include "mainwindow.h"
#include "resources.h"
#include "windows.h"

#include <QApplication>
#include <QClipboard>

int
main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Resources::load();
    Emojis::loadEmojis();

    ColorScheme::instance().setColors(0, -0.8);

    MainWindow &w = Windows::mainWindow();
    w.show();

    IrcManager::connect();

    return a.exec();
}
