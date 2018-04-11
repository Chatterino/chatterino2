#include "application.hpp"
#include "singletons/pathmanager.hpp"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QFile>
#include <QLibrary>
#include <QStringList>

#include <iostream>

#include "util/networkmanager.hpp"

#ifdef USEWINSDK
#include "util/nativeeventhelper.hpp"
#endif

#include "fstream"

#ifdef Q_OS_WIN
#include "fcntl.h"
#include "io.h"
#include "stdio.h"
#endif

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

    int ret = 0;

    {
        // Initialize application
        chatterino::Application app;

        // Start the application
        ret = app.run(a);

        // Application will go out of scope here and deinitialize itself
    }

    // Save settings
    pajlada::Settings::SettingManager::save();

    // Deinitialize NetworkManager (stop thread and wait for finish, should be instant)
    chatterino::util::NetworkManager::deinit();

    _exit(0);
}

void writeByteArray(QByteArray a)
{
    char *data = a.data();
    uint32_t size;
    size = a.size();
    std::cout.write(reinterpret_cast<char *>(&size), 4);
    std::cout.write(data, a.size());
}

void runNativeMessagingHost()
{
#ifdef Q_OS_WIN
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    //    std::ofstream xd("C:\\users\\daniel\\desktop\\xd.lmao");

    while (true) {
        char size_c[4];
        std::cin.read(size_c, 4);

        if (std::cin.eof()) {
            break;
        }

        uint32_t size = *reinterpret_cast<uint32_t *>(size_c);

        char *b = (char *)malloc(size + 1);
        std::cin.read(b, size);
        *(b + size) = '\0';

        //        xd << b;
        //        xd.flush();

        free(b);

        //        writeByteArray(QString("{\"xd\":1}").toUtf8());
    }
}
