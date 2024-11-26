#include "util/OnceFlag.hpp"

#include "Test.hpp"

#include <thread>

using namespace chatterino;

// this test shouldn't time out (no assert necessary)
TEST(OnceFlag, basic)
{
    OnceFlag startedFlag;
    OnceFlag startedAckFlag;
    OnceFlag stoppedFlag;

    std::thread t([&] {
        startedFlag.set();
        startedAckFlag.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        stoppedFlag.set();
    });

    startedFlag.wait();
    startedAckFlag.set();
    stoppedFlag.wait();

    t.join();
}

TEST(OnceFlag, waitFor)
{
    OnceFlag startedFlag;
    OnceFlag startedAckFlag;
    OnceFlag stoppedFlag;

    std::thread t([&] {
        startedFlag.set();
        startedAckFlag.wait();

        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        stoppedFlag.set();
    });

    startedFlag.wait();
    startedAckFlag.set();

    auto start = std::chrono::system_clock::now();
    ASSERT_TRUE(stoppedFlag.waitFor(std::chrono::milliseconds{200}));
    auto stop = std::chrono::system_clock::now();

    ASSERT_LT(stop - start, std::chrono::milliseconds{200});

    start = std::chrono::system_clock::now();
    ASSERT_TRUE(stoppedFlag.waitFor(std::chrono::milliseconds{1000}));
    stop = std::chrono::system_clock::now();

    ASSERT_LT(stop - start, std::chrono::milliseconds{10});

    start = std::chrono::system_clock::now();
    stoppedFlag.wait();
    stop = std::chrono::system_clock::now();

    ASSERT_LT(stop - start, std::chrono::milliseconds{10});

    t.join();
}

TEST(OnceFlag, waitForTimeout)
{
    OnceFlag startedFlag;
    OnceFlag startedAckFlag;
    OnceFlag stoppedFlag;

    std::thread t([&] {
        startedFlag.set();
        startedAckFlag.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        stoppedFlag.set();
    });

    startedFlag.wait();
    startedAckFlag.set();

    ASSERT_FALSE(stoppedFlag.waitFor(std::chrono::milliseconds{25}));
    stoppedFlag.wait();

    t.join();
}
