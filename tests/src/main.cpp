#include "common/NetworkManager.hpp"
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

    QTimer::singleShot(0, [&]() {
        auto res = RUN_ALL_TESTS();

        chatterino::NetworkManager::deinit();

        settingsDir.remove();
        QApplication::exit(res);
    });

    return QApplication::exec();
#else
    return RUN_ALL_TESTS();
#endif
}
