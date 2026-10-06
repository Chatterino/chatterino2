#include <chatterino-embed/App.hpp>
#include <chatterino-embed/AppBuilder.hpp>
#include <chatterino-embed/Split.hpp>
#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenuBar>

namespace {

auto setupChatterinoEmbed()
{
    chatterino::embed::AppBuilder builder;
    builder.setRootDirectory(qApp->applicationDirPath());
    builder.setSaveSettingsOnExit(false);
    return std::unique_ptr<chatterino::embed::App>{builder.createApp()};
}

}  // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    auto c2App = setupChatterinoEmbed();

    auto *mainWindow = new QMainWindow;
    mainWindow->resize({600, 400});

    mainWindow->menuBar()->addAction("Add split", [&] {
        auto *dock = new QDockWidget("Split", mainWindow);
        dock->setWidget(new chatterino::embed::Split(dock));
        mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dock);
    });

    mainWindow->show();

    QObject::connect(qApp, &QApplication::aboutToQuit, c2App.get(),
                     &chatterino::embed::App::aboutToQuit);

    return QApplication::exec();
}
