// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/Literals.hpp"
#include "controllers/filters/FilterSet.hpp"
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

class FilterMessages : public bench::MessageBenchmark
{
public:
    explicit FilterMessages(QString name, QStringList filters)
        : bench::MessageBenchmark(std::move(name))
        , filterTexts(std::move(filters))
    {
    }

    void run(benchmark::State &state) override
    {
        auto parsed = recentmessages::detail::parseRecentMessages(
            this->messages.object());
        auto built = recentmessages::detail::buildRecentMessages(
            parsed, this->chan.get());

        QMap<QUuid, FilterRecordPtr> filters;
        for (qsizetype i = 0; i < this->filterTexts.size(); i++)
        {
            // ensure deterministic order
            auto id = QUuid(static_cast<uint>(i), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            auto filter = std::make_shared<FilterRecord>(
                QString::number(i), this->filterTexts[i], id);
            if (!filter->valid())
            {
                qCDebug(chatterinoApp) << i << this->filterTexts[i];
                assert(false);
                continue;
            }
            filters.insert(id, filter);
        }
        assert(filters.size() == this->filterTexts.size());
        FilterSet set(std::move(filters));

        for (auto _ : state)
        {
            for (const auto &msg : built)
            {
                bool filtered = set.filter(msg, this->chan);
                benchmark::DoNotOptimize(filtered);
                benchmark::ClobberMemory();
            }
        }
    }

private:
    QStringList filterTexts;
};

void BM_FilterMessages(benchmark::State &state, QString channel,
                       QStringList filters)
{
    FilterMessages bench(std::move(channel), std::move(filters));
    bench.run(state);
}

}  // namespace

BENCHMARK_CAPTURE(
    BM_FilterMessages, nymn_modmessages, u"nymn"_s,
    {
        uR".(channel.name == "nymn" && author.badges contains "moderator")."_s,
    });

BENCHMARK_CAPTURE(
    BM_FilterMessages, nymn_mod_party, u"nymn"_s,
    {
        uR".((author.badges contains "moderator") && (message.content contains "forsenParty"))."_s,
    });

BENCHMARK_CAPTURE(BM_FilterMessages, nymn_len40_or_sub, u"nymn"_s,
                  {
                      uR".(message.length < 40 || author.subbed)."_s,
                  });

BENCHMARK_CAPTURE(BM_FilterMessages, nymn_no_sub, u"nymn"_s,
                  {
                      uR".(!flags.sub_message)."_s,
                  });

BENCHMARK_CAPTURE(BM_FilterMessages, nymn_with_color, u"nymn"_s,
                  {
                      uR".(!author.no_color)."_s,
                  });

BENCHMARK_CAPTURE(
    BM_FilterMessages, nymn_complex_regex, u"nymn"_s,
    {
        uR".((message.content match r"^(?!.*(?:my|complex|(re.*x))).*$"))."_s,
    });

BENCHMARK_CAPTURE(
    BM_FilterMessages, nymn_big_or, u"nymn"_s,
    {
        uR".((author.subbed && author.sub_length >= 6) || flags.system_message || flags.first_message || flags.automod || flags.sub_message)."_s,
    });

BENCHMARK_CAPTURE(
    BM_FilterMessages, nymn_with_color_and_edm_single, u"nymn"_s,
    {
        uR".(!author.no_color && message.content contains "EDM")."_s,
    });

BENCHMARK_CAPTURE(BM_FilterMessages, nymn_with_color_and_edm_separate,
                  u"nymn"_s,
                  {
                      uR".(!author.no_color)."_s,
                      uR".(message.content contains "EDM")."_s,
                  });
