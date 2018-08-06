#include "BrowserExtension.hpp"

#include "singletons/NativeMessaging.hpp"

#include <QStringList>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <memory>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

namespace chatterino {

namespace {
void initFileMode()
{
#ifdef Q_OS_WIN
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
}

void runLoop(NativeMessagingClient &client)
{
    while (true) {
        char size_c[4];
        std::cin.read(size_c, 4);

        if (std::cin.eof()) {
            break;
        }

        uint32_t size = *reinterpret_cast<uint32_t *>(size_c);

#if 0
    bool bigEndian = isBigEndian();
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

        std::unique_ptr<char[]> b(new char[size + 1]);
        std::cin.read(b.get(), size);
        *(b.get() + size) = '\0';

        client.sendMessage(
            QByteArray::fromRawData(b.get(), static_cast<int32_t>(size)));
    }
}
}  // namespace

bool shouldRunBrowserExtensionHost(const QStringList &args)
{
    return args.size() > 0 && (args[0].startsWith("chrome-extension://") ||
                               args[0].endsWith(".json"));
}

void runBrowserExtensionHost()
{
    initFileMode();

    std::atomic<bool> ping(false);

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&ping] {
        if (!ping.exchange(false)) {
            _Exit(0);
        }
    });
    timer.setInterval(11000);
    timer.start();

    NativeMessagingClient client;

    runLoop(client);
}

}  // namespace chatterino
