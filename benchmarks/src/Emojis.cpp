#include "providers/emoji/Emojis.hpp"

#include <benchmark/benchmark.h>
#include <QDebug>
#include <QString>

using namespace chatterino;

namespace {

std::shared_ptr<Emojis> getEmojis()
{
    static std::shared_ptr<Emojis> emojis = []() {
        auto *emojis = new Emojis();
        emojis->load();
        return std::make_shared<Emojis>(emojis);
    }();

    return emojis;
}

EmotePtr penguin()
{
    std::shared_ptr<EmojiData> penguin;
    getEmojis()->getEmojis().tryGet("1F427", penguin);
    return penguin->emote;
}

}  // namespace

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
        std::vector<boost::variant<EmotePtr, QString>> expectedOutput;
    };

    const auto &emojiMap = emojis.getEmojis();
    std::shared_ptr<EmojiData> penguin;
    emojiMap.tryGet("1F427", penguin);
    auto penguinEmoji = penguin->emote;

    std::vector<TestCase> tests{
        {
            // 1 emoji
            "foo 🐧 bar",
            // expected output
            {
                "foo ",
                penguinEmoji,
                " bar",
            },
        },
        {
            // no emoji
            "foo bar",
            // expected output
            {
                "foo bar",
            },
        },
        {
            // many emoji
            "foo 🐧 bar 🐧🐧🐧🐧🐧",
            // expected output
            {
                "foo ",
                penguinEmoji,
                " bar ",
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
                    if (v.type() == typeid(QString))
                    {
                        qDebug() << "output:" << boost::get<QString>(v);
                    }
                }
            }
        }
    }
}

BENCHMARK(BM_EmojiParsing);

template <class... Args>
static void BM_EmojiParsing2(benchmark::State &state, Args &&...args)
{
    Emojis emojis;

    emojis.load();

    struct TestCase {
        QString input;
        std::vector<boost::variant<EmotePtr, QString>> expectedOutput;
    };

    const auto &emojiMap = emojis.getEmojis();
    std::shared_ptr<EmojiData> penguin;
    emojiMap.tryGet("1F427", penguin);
    auto penguinEmoji = penguin->emote;

    std::vector<TestCase> tests{
        {
            // 1 emoji
            "foo 🐧 bar",
            // expected output
            {
                "foo ",
                penguinEmoji,
                " bar",
            },
        },
        {
            // no emoji
            "foo bar",
            // expected output
            {
                "foo bar",
            },
        },
        {
            // many emoji
            "foo 🐧 bar 🐧🐧🐧🐧🐧",
            // expected output
            {
                "foo ",
                penguinEmoji,
                " bar ",
                penguinEmoji,
                penguinEmoji,
                penguinEmoji,
                penguinEmoji,
                penguinEmoji,
            },
        },
    };

    auto argsTuple = std::make_tuple(std::move(args)...);
    auto input = std::get<0>(argsTuple);
    auto expectedOutput = std::get<1>(argsTuple);
    for (auto _ : state)
    {
        auto output = emojis.parse(input);

        bool areEqual =
            std::equal(output.begin(), output.end(), expectedOutput.begin());

        if (!areEqual)
        {
            qDebug() << "BAD BENCH";
            for (const auto &v : output)
            {
                if (v.type() == typeid(QString))
                {
                    qDebug() << "output:" << boost::get<QString>(v);
                }
            }
        }
    }
}

BENCHMARK_CAPTURE(BM_EmojiParsing2, "foo 🐧 bar",
                  {
                      "foo ",
                      penguin(),
                      " bar",
                  });
