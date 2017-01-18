#include "channels.h"
#include "colorscheme.h"
#include "emojis.h"
#include "ircmanager.h"
#include "resources.h"
#include "widgets/mainwindow.h"
#include "windows.h"

#include <QApplication>
#include <QClipboard>

using namespace chatterino;
using namespace chatterino::widgets;

int
main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Resources::load();
    Emojis::loadEmojis();

    ColorScheme::instance().setColors(0, -0.8);

    MainWindow &w = Windows::getMainWindow();
    w.show();

    IrcManager::connect();

    return a.exec();
}
