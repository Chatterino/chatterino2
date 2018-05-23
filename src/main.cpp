#include "application.hpp"
#include "singletons/nativemessagingmanager.hpp"
#include "singletons/pathmanager.hpp"
#include "util/networkmanager.hpp"
#include "widgets/lastruncrashdialog.hpp"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QFile>
#include <QLibrary>
#include <QStringList>
#include <QStyleFactory>

#ifdef USEWINSDK
#include "util/nativeeventhelper.hpp"
#endif

#include <fstream>
#include <iostream>

int runGui(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    // read args
    QStringList args;

    for (int i = 1; i < argc; i++) {
        args << argv[i];
    }

    // TODO: can be any argument
    if (args.size() > 0 && args[0].startsWith("chrome-extension://")) {
        chatterino::Application::instantiate(argc, argv);
        auto app = chatterino::getApp();
        app->construct();

        chatterino::Application::runNativeMessagingHost();
        return 0;
    }

    // run gui
    return runGui(argc, argv);
}

int runGui(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_Use96Dpi, true);
#ifdef Q_OS_WIN32
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling, true);
#endif
    //    QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
    QApplication a(argc, argv);

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // borrowed from
    // https://stackoverflow.com/questions/15035767/is-the-qt-5-dark-fusion-theme-available-for-windows
    QPalette darkPalette = a.palette();

    darkPalette.setColor(QPalette::Window, QColor(33, 33, 33));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Base, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Dark, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::Shadow, QColor(20, 20, 20));
    darkPalette.setColor(QPalette::Button, QColor(70, 70, 70));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));

    qApp->setPalette(darkPalette);

// Install native event handler for hidpi on windows
#ifdef USEWINSDK
    a.installNativeEventFilter(new chatterino::util::DpiNativeEventFilter);
#endif

    // Initialize NetworkManager
    chatterino::util::NetworkManager::init();

    // Initialize application
    chatterino::Application::instantiate(argc, argv);
    auto app = chatterino::getApp();

    app->construct();

    auto &pathMan = *app->paths;
    // Running file
    auto runningPath = pathMan.settingsFolderPath + "/running_" + pathMan.appPathHash;

    if (QFile::exists(runningPath)) {
#ifndef DISABLE_CRASH_DIALOG
        chatterino::widgets::LastRunCrashDialog dialog;
        dialog.exec();
#endif
    } else {
        QFile runningFile(runningPath);

        runningFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        runningFile.flush();
        runningFile.close();
    }

    app->initialize();

    // Start the application
    // This is a blocking call
    app->run(a);

    // We have finished our application, make sure we save stuff
    app->save();

    // Running file
    QFile::remove(runningPath);

    // Save settings
    pajlada::Settings::SettingManager::save();

    // Deinitialize NetworkManager (stop thread and wait for finish, should be instant)
    chatterino::util::NetworkManager::deinit();

    _exit(0);
}
