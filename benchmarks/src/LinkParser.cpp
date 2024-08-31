#include "common/LinkParser.hpp"

#include <benchmark/benchmark.h>
#include <QDebug>
#include <QString>
#include <QStringList>

using namespace chatterino;

const QString INPUT = QStringLiteral(
    "If your Chatterino isn't loading FFZ emotes, update to the latest nightly "
    "(or 2.4.2 if its out) "
    "https://github.com/Chatterino/chatterino2/releases/tag/nightly-build "
    "AlienPls https://www.youtube.com/watch?v=ELBBiBDcWc0 "
    "127.0.3 aaaa xd 256.256.256.256 AsdQwe xd 127.0.0.1 https://. "
    "*https://.be "
    "https://a: http://a.b (https://a.be) ftp://xdd.com "
    "this is a text lol . ://foo.com //aa.de :/foo.de xd.XDDDDDD ");

static void BM_LinkParsing(benchmark::State &state)
{
    QStringList words = INPUT.split(' ');

    // Make sure the TLDs are loaded
    {
        benchmark::DoNotOptimize(linkparser::parse("xd.com"));
    }

    for (auto _ : state)
    {
        for (const auto &word : words)
        {
            auto parsed = linkparser::parse(word);
            benchmark::DoNotOptimize(parsed);
        }
    }
}

BENCHMARK(BM_LinkParsing);
