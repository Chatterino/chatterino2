#include "util/RatelimitBucket.hpp"

#include "Test.hpp"

#include <QApplication>
#include <QDebug>
#include <QtConcurrent>

#include <chrono>
#include <thread>

using namespace chatterino;

TEST(RatelimitBucket, BatchTwoParts)
{
    const int cooldown = 100;
    int n = 0;
    auto cb = [&n](QString msg) {
        qDebug() << msg;
        ++n;
    };
    auto bucket = std::make_unique<RatelimitBucket>(5, cooldown, cb, nullptr);
    bucket->send("1");
    EXPECT_EQ(n, 1);

    bucket->send("2");
    EXPECT_EQ(n, 2);

    bucket->send("3");
    EXPECT_EQ(n, 3);

    bucket->send("4");
    EXPECT_EQ(n, 4);

    bucket->send("5");
    EXPECT_EQ(n, 5);

    bucket->send("6");
    // Rate limit reached, n will not have changed yet. If we wait for the cooldown to run, n should have changed
    EXPECT_EQ(n, 5);

    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds{cooldown});
    QCoreApplication::processEvents();

    EXPECT_EQ(n, 6);
}
