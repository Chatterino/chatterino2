// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/Literals.hpp"
#include "MessageBuilding.hpp"
#include "providers/recentmessages/Impl.hpp"

#include <benchmark/benchmark.h>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>

using namespace chatterino;
using namespace Qt::Literals;

namespace {

class ParseRecentMessages : public bench::MessageBenchmark
{
public:
    explicit ParseRecentMessages(QString name)
        : bench::MessageBenchmark(std::move(name))
    {
    }

    void run(benchmark::State &state) override
    {
        for (auto _ : state)
        {
            auto parsed = recentmessages::detail::parseRecentMessages(
                this->messages.object());
            benchmark::DoNotOptimize(parsed);
        }
    }
};

class BuildRecentMessages : public bench::MessageBenchmark
{
public:
    explicit BuildRecentMessages(QString name)
        : bench::MessageBenchmark(std::move(name))
    {
    }

    void run(benchmark::State &state) override
    {
        auto parsed = recentmessages::detail::parseRecentMessages(
            this->messages.object());
        for (auto _ : state)
        {
            auto built = recentmessages::detail::buildRecentMessages(
                parsed, this->chan.get());
            benchmark::DoNotOptimize(built);
        }
    }
};

void BM_ParseRecentMessages(benchmark::State &state, const QString &name)
{
    ParseRecentMessages bench(name);
    bench.run(state);
}

void BM_BuildRecentMessages(benchmark::State &state, const QString &name)
{
    BuildRecentMessages bench(name);
    bench.run(state);
}

}  // namespace

BENCHMARK_CAPTURE(BM_ParseRecentMessages, nymn, u"nymn"_s);
BENCHMARK_CAPTURE(BM_BuildRecentMessages, nymn, u"nymn"_s);
