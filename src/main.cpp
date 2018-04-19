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

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

bool isBigEndian()
{
    int test = 1;
    char *p = (char *)&test;

    return p[0] == 0;
}

void runNativeMessagingHost();
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
        runNativeMessagingHost();
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

    // Initialize settings
    bool success = chatterino::singletons::PathManager::getInstance().init(argc, argv);
    if (!success) {
        printf("Error initializing paths\n");
        return 1;
    }

    // Initialize NetworkManager
    chatterino::util::NetworkManager::init();

    // Running file
    auto &pathMan = chatterino::singletons::PathManager::getInstance();
    auto runningPath = pathMan.settingsFolderPath + "/running_" + pathMan.appPathHash;

    if (QFile::exists(runningPath)) {
        chatterino::widgets::LastRunCrashDialog dialog;
        dialog.exec();
    } else {
        QFile runningFile(runningPath);

        runningFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        runningFile.flush();
        runningFile.close();
    }

    // Application
    {
        // Initialize application
        chatterino::Application app;

        // Start the application
        app.run(a);

        // Application will go out of scope here and deinitialize itself
    }

    // Running file
    QFile::remove(runningPath);

    // Save settings
    pajlada::Settings::SettingManager::save();

    // Deinitialize NetworkManager (stop thread and wait for finish, should be instant)
    chatterino::util::NetworkManager::deinit();

    _exit(0);
}

void runNativeMessagingHost()
{
#ifdef Q_OS_WIN
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    auto &nmm = chatterino::singletons::NativeMessagingManager::getInstance();

#if 0
    bool bigEndian = isBigEndian();
#endif

    while (true) {
        char size_c[4];
        std::cin.read(size_c, 4);

        if (std::cin.eof()) {
            break;
        }

        uint32_t size = *reinterpret_cast<uint32_t *>(size_c);
#if 0
        // To avoid breaking strict-aliasing rules and potentially inducing undefined behaviour, the following code can be run instead
        uint32_t size = 0;
        if (bigEndian) {
            size = size_c[3] | static_cast<uint32_t>(size_c[2]) << 8 |
                   static_cast<uint32_t>(size_c[1]) << 16 | static_cast<uint32_t>(size_c[0]) << 24;
        } else {
            size = size_c[0] | static_cast<uint32_t>(size_c[1]) << 8 |
                   static_cast<uint32_t>(size_c[2]) << 16 | static_cast<uint32_t>(size_c[3]) << 24;
        }
#endif

        char *b = (char *)malloc(size + 1);
        std::cin.read(b, size);
        *(b + size) = '\0';

        nmm.sendToGuiProcess(QByteArray(b, size));

        free(b);
    }
}
