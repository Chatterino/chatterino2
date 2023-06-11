#include "common/NetworkManager.hpp"
#include "common/Version.hpp"
#include "singletons/Settings.hpp"

#include <gtest/gtest.h>
#include <QApplication>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QtConcurrent>
#include <QTimer>

#include <chrono>
#include <thread>

using namespace chatterino;

CHATTERINO_DECLARE_BUILD_CONSTANTS()

#define SUPPORT_QT_NETWORK_TESTS

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

#ifdef SUPPORT_QT_NETWORK_TESTS
    QApplication app(argc, argv);
    // make sure to always debug-log
    QLoggingCategory::setFilterRules("*.debug=true");

    chatterino::NetworkManager::init();

    // Ensure settings are initialized before any tests are run
    QTemporaryDir settingsDir;
    settingsDir.setAutoRemove(false);  // we'll remove it manually
    qDebug() << "Settings directory:" << settingsDir.path();
    chatterino::Settings settings(settingsDir.path());

    QtConcurrent::run([&app, &settingsDir]() mutable {
        auto res = RUN_ALL_TESTS();

        chatterino::NetworkManager::deinit();

        settingsDir.remove();
        app.exit(res);
    });

    return app.exec();
#else
    return RUN_ALL_TESTS();
#endif
}
