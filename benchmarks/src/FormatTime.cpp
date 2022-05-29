#include "util/FormatTime.hpp"

#include <benchmark/benchmark.h>

using namespace chatterino;

template <class... Args>
void BM_TimeFormatting(benchmark::State &state, Args &&...args)
{
    auto args_tuple = std::make_tuple(std::move(args)...);
    for (auto _ : state)
    {
        formatTime(std::get<0>(args_tuple));
    }
}

BENCHMARK_CAPTURE(BM_TimeFormatting, 0, 0);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs0, "0");
BENCHMARK_CAPTURE(BM_TimeFormatting, 1337, 1337);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs1337, "1337");
BENCHMARK_CAPTURE(BM_TimeFormatting, 623452, 623452);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs623452, "623452");
BENCHMARK_CAPTURE(BM_TimeFormatting, 8345, 8345);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs8345, "8345");
BENCHMARK_CAPTURE(BM_TimeFormatting, 314034, 314034);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs314034, "314034");
BENCHMARK_CAPTURE(BM_TimeFormatting, 27, 27);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs27, "27");
BENCHMARK_CAPTURE(BM_TimeFormatting, 34589, 34589);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs34589, "34589");
BENCHMARK_CAPTURE(BM_TimeFormatting, 3659, 3659);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs3659, "3659");
BENCHMARK_CAPTURE(BM_TimeFormatting, 1045345, 1045345);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs1045345, "1045345");
BENCHMARK_CAPTURE(BM_TimeFormatting, 86432, 86432);
BENCHMARK_CAPTURE(BM_TimeFormatting, qs86432, "86432");
BENCHMARK_CAPTURE(BM_TimeFormatting, qsempty, "");
BENCHMARK_CAPTURE(BM_TimeFormatting, qsinvalid, "asd");
