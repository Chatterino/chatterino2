#include "util/FormatTime.hpp"

#include <benchmark/benchmark.h>

using namespace chatterino;

void BM_TimeFormattingQString(benchmark::State &state, const QString &v)
{
    for (auto _ : state)
    {
        formatTime(v);
    }
}

void BM_TimeFormattingInt(benchmark::State &state, int v)
{
    for (auto _ : state)
    {
        formatTime(v);
    }
}

BENCHMARK_CAPTURE(BM_TimeFormattingInt, 0, 0);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 1045345, 1045345);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 1337, 1337);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 27, 27);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 314034, 314034);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 34589, 34589);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 3659, 3659);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 623452, 623452);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 8345, 8345);
BENCHMARK_CAPTURE(BM_TimeFormattingInt, 86432, 86432);
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs0, "0");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs1045345, "1045345");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs1337, "1337");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs27, "27");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs314034, "314034");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs34589, "34589");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs3659, "3659");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs623452, "623452");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs8345, "8345");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qs86432, "86432");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qsempty, "");
BENCHMARK_CAPTURE(BM_TimeFormattingQString, qsinvalid, "asd");
