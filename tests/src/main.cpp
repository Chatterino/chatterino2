#include "common/NetworkManager.hpp"
#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "providers/twitch/api/Helix.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QtConcurrent>

#include <chrono>
#include <thread>
#include "common/NetworkManager.hpp"

#include <gtest/gtest.h>
#include <QApplication>
#include <QTimer>

using namespace chatterino;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    QApplication app(argc, argv);

    chatterino::NetworkManager::init();

    QtConcurrent::run([&app] {
        auto res = RUN_ALL_TESTS();

        chatterino::NetworkManager::deinit();

        app.exit(res);
    });

    return app.exec();
}
