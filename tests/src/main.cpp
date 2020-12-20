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

void xd()
{
    std::mutex mut;
    bool requestDone = false;
    std::condition_variable requestDoneCondition;

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    using namespace std::chrono_literals;

    auto url = "http://localhost:8000/status/200";
    NetworkRequest(url)
        .onSuccess([&](NetworkResult result) -> Outcome {
            qDebug() << "ON SUCCESS";
            qDebug() << url;
            return Success;
        })
        .onError([&](NetworkResult result) {
            qDebug() << "ON ERROR";
        })
        .execute();

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    EXPECT_TRUE(true);
}

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
