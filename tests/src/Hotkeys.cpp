#include "controllers/hotkeys/HotkeyHelpers.hpp"
#include "Test.hpp"

#include <vector>

using namespace chatterino;

struct argumentTest {
    const char *label;
    QString input;
    std::vector<QString> expected;
};

TEST(HotkeyHelpers, parseHotkeyArguments)
{
    std::vector<argumentTest> tests{
        {
            "Empty input must result in an empty vector",
            "",
            {},
        },
        {
            "Leading and trailing newlines/spaces are removed",
            "\n",
            {},
        },
        {
            "Single argument",
            "foo",
            {"foo"},
        },
        {
            "Single argument with trailing space trims the space",
            "foo ",
            {"foo"},
        },
        {
            "Single argument with trailing newline trims the newline",
            "foo\n",
            {"foo"},
        },
        {
            "Multiple arguments with leading and trailing spaces trims them",
            " foo \n bar \n baz ",
            {"foo", "bar", "baz"},
        },
        {
            "Multiple trailing newlines are trimmed",
            "foo\n\n",
            {"foo"},
        },
        {
            "Leading newline is trimmed",
            "\nfoo",
            {"foo"},
        },
        {
            "Leading newline + space trimmed",
            "\n foo",
            {"foo"},
        },
        {
            "Multiple leading newline trimmed",
            "\n\nfoo",
            {"foo"},
        },
        {
            "2 rows results in 2 vectors",
            "foo\nbar",
            {"foo", "bar"},
        },
        {
            "Multiple newlines in the middle are not trimmed",
            "foo\n\nbar",
            {"foo", "", "bar"},
        },
    };

    for (const auto &[label, input, expected] : tests)
    {
        auto output = parseHotkeyArguments(input);

        EXPECT_EQ(output, expected) << label;
    }
}
