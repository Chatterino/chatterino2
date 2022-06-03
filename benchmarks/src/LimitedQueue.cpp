#include "messages/LimitedQueue.hpp"
#include <benchmark/benchmark.h>

#include <boost/circular_buffer.hpp>

#include <deque>
#include <memory>
#include <numeric>

using namespace chatterino;

void BM_LimitedQueue_PushBack(benchmark::State &state)
{
    for (auto _ : state)
    {
        LimitedQueue<int> queue(1000);
        int d;
        for (int i = 0; i < 2000; ++i)
        {
            queue.pushBack(i, d);
        }
    }
}

void BM_LimitedQueue_PushFront(benchmark::State &state)
{
    std::vector<int> items;
    items.resize(10000);
    std::iota(items.begin(), items.end(), 0);

    for (auto _ : state)
    {
        state.PauseTiming();
        LimitedQueue<int> queue(1000);
        state.ResumeTiming();
        queue.pushFront(items);
    }
}

void BM_LimitedQueue_Replace(benchmark::State &state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        LimitedQueue<int> queue(1000);
        int d;
        for (int i = 0; i < 1000; ++i)
        {
            queue.pushBack(i, d);
        }
        state.ResumeTiming();

        for (int i = 0; i < 1000; ++i)
        {
            queue.replaceItem(i, 2000 + i);
        }
    }
}

void BM_LimitedQueue_Snapshot(benchmark::State &state)
{
    LimitedQueue<std::shared_ptr<int>> queue(1000);
    std::shared_ptr<int> d;
    for (int i = 0; i < 10000; ++i)
    {
        queue.pushBack(std::make_shared<int>(i), d);
    }

    for (auto _ : state)
    {
        auto snapshot = queue.getSnapshot();
        for (size_t i = 0; i < snapshot.size(); ++i)
        {
            benchmark::DoNotOptimize(snapshot[i]);
            queue.pushBack(std::make_shared<int>(i), d);
        }
    }
}

BENCHMARK(BM_LimitedQueue_PushBack);
BENCHMARK(BM_LimitedQueue_PushFront);
BENCHMARK(BM_LimitedQueue_Replace);
BENCHMARK(BM_LimitedQueue_Snapshot);
