#include <QApplication>
#include "mainwindow.h"
#include "colorscheme.h"
#include "ircmanager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ColorScheme::getInstance().setColors(0, -0.8);

    MainWindow w;
    w.show();

    Channel::addChannel("ian678");
    Channel::addChannel("fourtf");

    IrcManager::connect();

    return a.exec();
}
