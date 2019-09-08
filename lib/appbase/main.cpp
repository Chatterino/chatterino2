#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "ABSettings.hpp"
#include "ABTheme.hpp"
#include "singletons/Fonts.hpp"
#include "widgets/BaseWindow.hpp"

int main(int argc, char *argv[])
{
    using namespace AB_NAMESPACE;

    QApplication a(argc, argv);

    auto path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    qDebug() << path;

    QDir(path).mkdir(".");

    new Settings(path);
    new Fonts();

    BaseWindow widget(nullptr, BaseWindow::EnableCustomFrame);
    widget.setWindowTitle("asdf");
    widget.show();

    return a.exec();
}
