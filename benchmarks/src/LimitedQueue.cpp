#include "messages/LimitedQueue.hpp"

#include <benchmark/benchmark.h>

#include <memory>
#include <numeric>
#include <vector>

using namespace chatterino;

void BM_LimitedQueue_PushBack(benchmark::State &state)
{
    LimitedQueue<int> queue(1000);
    for (auto _ : state)
    {
        queue.pushBack(1);
    }
}

void BM_LimitedQueue_PushFront_One(benchmark::State &state)
{
    std::vector<int> items = {1};
    LimitedQueue<int> queue(2);

    for (auto _ : state)
    {
        state.PauseTiming();
        queue.clear();
        state.ResumeTiming();
        queue.pushFront(items);
    }
}

void BM_LimitedQueue_PushFront_Many(benchmark::State &state)
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
    LimitedQueue<int> queue(1000);
    for (int i = 0; i < 1000; ++i)
    {
        queue.pushBack(i);
    }

    for (auto _ : state)
    {
        queue.replaceItem(500, 500);
    }
}

void BM_LimitedQueue_Snapshot(benchmark::State &state)
{
    LimitedQueue<int> queue(1000);
    for (int i = 0; i < 1000; ++i)
    {
        queue.pushBack(i);
    }

    for (auto _ : state)
    {
        auto snapshot = queue.getSnapshot();
        benchmark::DoNotOptimize(snapshot);
    }
}

void BM_LimitedQueue_Snapshot_ExpensiveCopy(benchmark::State &state)
{
    LimitedQueue<std::shared_ptr<int>> queue(1000);
    for (int i = 0; i < 1000; ++i)
    {
        queue.pushBack(std::make_shared<int>(i));
    }

    for (auto _ : state)
    {
        auto snapshot = queue.getSnapshot();
        benchmark::DoNotOptimize(snapshot);
    }
}

void BM_LimitedQueue_Find(benchmark::State &state)
{
    LimitedQueue<int> queue(1000);
    for (int i = 0; i < 10000; ++i)
    {
        queue.pushBack(i);
    }

    for (auto _ : state)
    {
        auto res = queue.find([](const auto &val) {
            return val == 500;
        });
        benchmark::DoNotOptimize(res);
    }
}

BENCHMARK(BM_LimitedQueue_PushBack);
BENCHMARK(BM_LimitedQueue_PushFront_One);
BENCHMARK(BM_LimitedQueue_PushFront_Many);
BENCHMARK(BM_LimitedQueue_Replace);
BENCHMARK(BM_LimitedQueue_Snapshot);
BENCHMARK(BM_LimitedQueue_Snapshot_ExpensiveCopy);
BENCHMARK(BM_LimitedQueue_Find);
