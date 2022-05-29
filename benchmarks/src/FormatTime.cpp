#include "util/FormatTime.hpp"

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include <QDebug>

using namespace chatterino;

struct TestCase {
    int input;
    QString expectedOutput;
};

std::vector<TestCase> tests{
    {
        0,
        "0s",
    },
    {
        1337,
        "22m 17s",
    },
    {
        623452,
        "7d 5h 10m 52s",
    },
    {
        8345,
        "2h 19m 5s",
    },
    {
        314034,
        "3d 15h 13m 54s",
    },
    {
        27,
        "27s",
    },
    {
        34589,
        "9h 36m 29s",
    },
    {
        3659,
        "1h 59s",
    },
    {
        1045345,
        "12d 2h 22m 25s",
    },
    {
        86432,
        "1d 32s",
    },
};

static void BM_TimeFormatting(benchmark::State &state)
{
    for (auto _ : state)
    {
        for (const auto &test : tests)
        {
            auto output = formatTime(test.input);
            auto matches = output == test.expectedOutput;
        }
    }
}

BENCHMARK(BM_TimeFormatting);
