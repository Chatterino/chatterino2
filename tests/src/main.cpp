#include "common/Args.hpp"
#include "common/network/NetworkManager.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "Test.hpp"

#include <QApplication>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QtConcurrent>
#include <QTimer>

#include <chrono>
#include <thread>

using namespace chatterino;

#define SUPPORT_QT_NETWORK_TESTS

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

#ifdef SUPPORT_QT_NETWORK_TESTS
    QApplication app(argc, argv);
    // make sure to always debug-log
    QLoggingCategory::setFilterRules("chatterino.*=true");

    initResources();

    chatterino::NetworkManager::init();

    Args args;

    // Ensure settings are initialized before any tests are run
    QTemporaryDir settingsDir;
    settingsDir.setAutoRemove(false);  // we'll remove it manually
    chatterino::Settings settings(args, settingsDir.path());

    QTimer::singleShot(0, [&]() {
        auto res = RUN_ALL_TESTS();

        chatterino::NetworkManager::deinit();

        settingsDir.remove();

        // Pick up the last events from the eventloop
        // Using a loop to catch events queueing other events (e.g. deletions)
        for (size_t i = 0; i < 32; i++)
        {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }

        QApplication::exit(res);
    });

    return QApplication::exec();
#else
    return RUN_ALL_TESTS();
#endif
}
