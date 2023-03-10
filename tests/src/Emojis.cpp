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
            << "Input " << test.input.toStdString() << " failed";
    }
}
