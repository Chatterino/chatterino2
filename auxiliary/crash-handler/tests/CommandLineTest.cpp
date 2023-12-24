#include "CommandLine.hpp"

#include <gtest/gtest.h>

using namespace std::string_literals;

TEST(CommandLineTest, splitEncodedChatterinoArgs)
{
    struct TestCase {
        std::wstring input;
        std::vector<std::wstring> output;
    };

    std::initializer_list<TestCase> testCases{
        {
            L"-c+t:alien+--safe-mode"s,
            {L"-c"s, L"t:alien"s, L"--safe-mode"s},
        },
        {
            L"-c+t:++++++breaking news++++++!!+-V"s,
            {L"-c"s, L"t:+++breaking news+++!!"s, L"-V"s},
        },
        {
            L"++"s,
            {L"+"s},
        },
        {
            L""s,
            {},
        },
        {
            L"--channels=t:foo;t:bar;t:++++++foo++++++"s,
            {L"--channels=t:foo;t:bar;t:+++foo+++"},
        },
    };

    for (const auto &testCase : testCases)
    {
        EXPECT_EQ(splitEncodedChatterinoArgs(testCase.input), testCase.output);
    }
}
