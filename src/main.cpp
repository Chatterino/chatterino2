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
