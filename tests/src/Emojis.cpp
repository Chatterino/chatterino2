#include "providers/emoji/Emojis.hpp"

#include <gtest/gtest.h>
#include <QDebug>
#include <QString>

using namespace chatterino;

TEST(Emojis, ShortcodeParsing)
{
    Emojis emojis;

    emojis.load();

    struct TestCase {
        QString input;
        QString expectedOutput;
    };

    std::vector<TestCase> tests{
        {
            .input = "foo :penguin: bar",
            .expectedOutput = "foo 🐧 bar",
        },
        {
            .input = "foo :nonexistantcode: bar",
            .expectedOutput = "foo :nonexistantcode: bar",
        },
        {
            .input = ":male-doctor:",
            .expectedOutput = "👨‍⚕️",
        },
    };

    for (const auto &test : tests)
    {
        auto output = emojis.replaceShortCodes(test.input);

        EXPECT_EQ(output, test.expectedOutput)
            << "Input " << test.input.toStdString() << " failed";
    }
}
