#include "providers/emoji/Emojis.hpp"

#include "common/Literals.hpp"
#include "Test.hpp"

#include <QDebug>
#include <QString>

using namespace chatterino;
using namespace literals;

TEST(Emojis, ShortcodeParsing)
{
    Emojis emojis;

    emojis.load();

    struct TestCase {
        QString input;
        QString expectedOutput;
    };

    const std::vector<TestCase> tests{
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

    for (const auto &test : tests)
    {
        auto output = emojis.replaceShortCodes(test.input);

        auto matches = output == test.expectedOutput;
        if (!matches && !output.endsWith(QChar(0xFE0F)))
        {
            // Try to append 0xFE0F if needed
            output = output.append(QChar(0xFE0F));
        }

        EXPECT_EQ(output, test.expectedOutput)
            << "Input " << test.input << " failed";
    }
}

TEST(Emojis, Parse)
{
    Emojis emojis;

    emojis.load();

    struct TestCase {
        QString input;
        std::vector<boost::variant<EmotePtr, QString>> expectedOutput;
    };

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

    auto penguin = getEmoji("1F427");
    auto cool = getEmoji("1F192");
    auto skinTone6 = getEmoji("1F3FF");
    auto england = getEmoji("1F3F4-E0067-E0062-E0065-E006E-E0067-E007F");
    auto womanRunningtone2 = getEmoji("1F3C3-1F3FC-200D-2640-FE0F");
    auto kissWomanManTone1 =
        getEmoji("1F468-1F3FB-200D-2764-FE0F-200D-1F48B-200D-1F468-1F3FB");
    auto heavyEqualsSign = getEmoji("1F7F0");
    auto coupleKissTone1Tone2 =
        getEmoji("1F9D1-1F3FB-200D-2764-FE0F-200D-1F48B-200D-1F9D1-1F3FC");
    auto hearHands = getEmoji("1FAF6");

    const std::vector<TestCase> tests{
        {
            "abc",
            {"abc"},
        },
        {
            "abc def",
            {"abc def"},
        },
        {
            "abc🐧def",
            {"abc", penguin, "def"},
        },
        {
            "abc 🐧def",
            {"abc ", penguin, "def"},
        },
        {
            " abc🐧 def ",
            {" abc", penguin, " def "},
        },
        {
            "🐧",
            {penguin},
        },
        {
            "🐧🐧🐧🐧",
            {penguin, penguin, penguin, penguin},
        },
        {
            // england
            u"\U0001F3F4\U000E0067\U000E0062\U000E0065\U000E006E\U000E0067\U000E007F"_s
            // cool
            "\U0001F192"
            // skin tone 6
            "\U0001F3FF"
            // woman running tone2
            "\U0001F3C3\U0001F3FC\u200D\u2640\uFE0F"
            // [running] non-qualified
            "\U0001F3C3\U0001F3FC\u200D\u2640",
            {england, cool, skinTone6, womanRunningtone2, womanRunningtone2},
        },
        {
            // kiss woman tone1 man tone 1
            u"\U0001F468\U0001F3FB\u200D\u2764\uFE0F\u200D\U0001F48B\u200D\U0001F468\U0001F3FB"_s
            // [kiss] non-qualified
            "\U0001F468\U0001F3FB\u200D\u2764\u200D\U0001F48B\u200D"
            "\U0001F468"
            "\U0001F3FB"
            // heavy equals sign
            "\U0001F7F0",
            {kissWomanManTone1, kissWomanManTone1, heavyEqualsSign},
        },
        {
            // couple kiss tone 1, tone 2
            u"\U0001F9D1\U0001F3FB\u200D\u2764\uFE0F\u200D\U0001F48B\u200D\U0001F9D1\U0001F3FC"_s
            // [kiss] non-qualified
            "\U0001F9D1\U0001F3FB\u200D\u2764\u200D\U0001F48B\u200D\U0001F9D1"
            "\U0001F3FC"
            // heart hands
            "\U0001FAF6",
            {coupleKissTone1Tone2, coupleKissTone1Tone2, hearHands},
        },
    };

    for (const auto &test : tests)
    {
        auto output = emojis.parse(test.input);

        // can't use EXPECT_EQ because EmotePtr can't be printed
        if (output != test.expectedOutput)
        {
            EXPECT_TRUE(false) << "Input " << test.input << " failed";
        }
    }
}
