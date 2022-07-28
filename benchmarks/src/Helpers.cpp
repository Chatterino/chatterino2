#include "util/Helpers.hpp"

#include <benchmark/benchmark.h>

using namespace chatterino;

template <class... Args>
void BM_SplitListIntoBatches(benchmark::State &state, Args &&...args)
{
    auto args_tuple = std::make_tuple(std::move(args)...);
    for (auto _ : state)
    {
        auto result = splitListIntoBatches(std::get<0>(args_tuple),
                                           std::get<1>(args_tuple));
        assert(!result.empty());
    }
}

BENCHMARK_CAPTURE(BM_SplitListIntoBatches, 7 / 3,
                  QStringList{"", "", "", "", "", "", ""}, 3);
BENCHMARK_CAPTURE(BM_SplitListIntoBatches, 6 / 3,
                  QStringList{"", "", "", "", "", ""}, 3);
BENCHMARK_CAPTURE(BM_SplitListIntoBatches, 100 / 3,
                  QStringList{
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "",
                  },
                  3);
BENCHMARK_CAPTURE(BM_SplitListIntoBatches, 100 / 49,
                  QStringList{
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "", "", "", "", "",
                      "", "", "", "", "", "", "", "", "",
                  },
                  49);
