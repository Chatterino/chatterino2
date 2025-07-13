#include "common/UniqueAccess.hpp"
#include "Test.hpp"

#include <QDebug>
#include <QString>

#include <chrono>
#include <random>
#include <thread>

using namespace chatterino;

using namespace std::chrono_literals;

TEST(AccessGuardLocker, NonConcurrentUsage)
{
    std::shared_mutex m;
    int e = 0;

    {
        AccessGuard<int> guard(e, m);
        *guard = 3;
    }
    EXPECT_EQ(e, 3);

    {
        AccessGuard<int> guard(e, m);
        *guard = 4;
    }
    EXPECT_EQ(e, 4);

    {
        SharedAccessGuard<int> guard(e, m);
        EXPECT_EQ(*guard, 4);
    }
    EXPECT_EQ(e, 4);
}

TEST(AccessGuardLocker, ConcurrentUsage)
{
    // This test doesn't actually prove anything on normal use, rather it needs to be run with AddressSanitizer/ThreadSanitizer and not error out for this to give any confidence
    std::shared_mutex m;
    int e = 0;

    auto w = [&e, &m] {
        std::mt19937_64 eng{std::random_device{}()};
        std::uniform_int_distribution<> dist{1, 4};
        std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
        if (rand() % 2 == 0)
        {
            AccessGuard<int> guard(e, m);
            std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
            *guard += 1;
        }
        else
        {
            SharedAccessGuard<int> guard(e, m);
            std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
            volatile int hehe = *guard;
            (void)hehe;
        }
    };

    std::vector<std::thread> threads;

    for (int i = 0; i < 500; ++i)
    {
        threads.emplace_back(w);
    }

    for (auto &t : threads)
    {
        t.join();
    }
}
