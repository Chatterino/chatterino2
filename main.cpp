#include <QApplication>
#include "mainwindow.h"
#include "colorscheme.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //ColorScheme::makeScheme(0, -0.8);

    MainWindow w;
    w.show();

    return a.exec();
}
