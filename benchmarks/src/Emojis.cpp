#include "providers/emoji/Emojis.hpp"

#include <benchmark/benchmark.h>
#include <QDebug>
#include <QString>

using namespace chatterino;

static void BM_ShortcodeParsing(benchmark::State &state)
{
    Emojis emojis;

    emojis.load();

    struct TestCase {
        QString input;
        QString expectedOutput;
    };

    std::vector<TestCase> tests{
        {
            // input
            "foo :penguin: bar",
            // expected output
            "foo 🐧 bar",
        },
        {
            // input
            "foo :nonexistantcode: bar",
            // expected output
            "foo :nonexistantcode: bar",
        },
        {
            // input
            ":male-doctor:",
            // expected output
            "👨‍⚕️",
        },
    };

    for (auto _ : state)
    {
        for (const auto &test : tests)
        {
            auto output = emojis.replaceShortCodes(test.input);

            auto matches = output == test.expectedOutput;
            if (!matches && !output.endsWith(QChar(0xFE0F)))
            {
                // Try to append 0xFE0F if needed
                output = output.append(QChar(0xFE0F));
            }
        }
    }
}

BENCHMARK(BM_ShortcodeParsing);
