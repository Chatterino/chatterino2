#include "common/NetworkManager.hpp"
#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/api/Helix.hpp"

#include <gtest/gtest.h>
#include <QApplication>
#include <QJsonArray>
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

    chatterino::NetworkManager::init();

    QtConcurrent::run([&app] {
        auto res = RUN_ALL_TESTS();

        chatterino::NetworkManager::deinit();

        app.exit(res);
    });

    return app.exec();
#else
    return RUN_ALL_TESTS();
#endif
}
