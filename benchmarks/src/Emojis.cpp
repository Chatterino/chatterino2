// SPDX-FileCopyrightText: 2021 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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

static void BM_EmojiParsing(benchmark::State &state)
{
    Emojis emojis;

    emojis.load();

    struct TestCase {
        QString input;
        std::vector<std::variant<EmotePtr, QStringView>> expectedOutput;
    };

    const auto &emojiMap = emojis.getEmojis();
    auto getEmoji = [&](auto code) {
        std::shared_ptr<EmojiData> emoji;
        for (const auto &e : emojis.getEmojis())
        {
            if (e->unifiedCode == code)
            {
                emoji = e;
                break;
            }
        }
        return emoji->emote;
    };
    auto penguinEmoji = getEmoji("1F427");
    assert(penguinEmoji.get() != nullptr);

    std::vector<TestCase> tests{
        {
            // 1 emoji
            "foo 🐧 bar",
            // expected output
            {
                u"foo ",
                penguinEmoji,
                u" bar",
            },
        },
        {
            // no emoji
            "foo bar",
            // expected output
            {
                u"foo bar",
            },
        },
        {
            // many emoji
            "foo 🐧 bar 🐧🐧🐧🐧🐧",
            // expected output
            {
                u"foo ",
                penguinEmoji,
                u" bar ",
                penguinEmoji,
                penguinEmoji,
                penguinEmoji,
                penguinEmoji,
                penguinEmoji,
            },
        },
    };

    for (auto _ : state)
    {
        for (const auto &test : tests)
        {
            auto output = emojis.parse(test.input);

            bool areEqual = std::equal(output.begin(), output.end(),
                                       test.expectedOutput.begin());

            if (!areEqual)
            {
                qDebug() << "BAD BENCH";
                for (const auto &v : output)
                {
                    if (std::holds_alternative<QStringView>(v))
                    {
                        qDebug() << "output:" << std::get<QStringView>(v);
                    }
                }
            }
        }
    }
}

BENCHMARK(BM_EmojiParsing);

static void BM_EmojiParsing2(benchmark::State &state, const QString &input,
                             int expectedNumEmojis)
{
    Emojis emojis;

    emojis.load();

    for (auto _ : state)
    {
        auto output = emojis.parse(input);
        int actualNumEmojis = 0;
        for (const auto &part : output)
        {
            if (std::holds_alternative<EmotePtr>(part))
            {
                ++actualNumEmojis;
            }
        }

        if (actualNumEmojis != expectedNumEmojis)
        {
            qDebug() << "BAD BENCH, EXPECTED NUM EMOJIS IS WRONG"
                     << actualNumEmojis;
        }
    }
}

BENCHMARK_CAPTURE(BM_EmojiParsing2, one_emoji, "foo 🐧 bar", 1);
BENCHMARK_CAPTURE(BM_EmojiParsing2, two_emoji, "foo 🐧 bar 🐧", 2);
BENCHMARK_CAPTURE(
    BM_EmojiParsing2, many_emoji,
    "😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 "
    "😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 "
    "😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 😂 ",
    61);
