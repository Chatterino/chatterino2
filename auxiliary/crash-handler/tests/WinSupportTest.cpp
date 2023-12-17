#include "WinSupport.hpp"

#include <gtest/gtest.h>
#include <util/win/exception_codes.h>
#include <Windows.h>

using namespace std::string_literals;

TEST(WinSupport, formatCommonException)
{
    struct TestCase {
        uint32_t input = 0;
        std::optional<std::wstring> output;
    };

    std::initializer_list<TestCase> testCases{
        {
            EXCEPTION_ACCESS_VIOLATION,
            L"ExceptionAccessViolation"s,
        },
        {
            EXCEPTION_INVALID_HANDLE,
            L"ExceptionInvalidHandle"s,
        },
        {
            0x5,
            std::nullopt,
        },
        {
            0xff,
            std::nullopt,
        },
        {
            0x100,
            std::nullopt,
        },
        {
            crashpad::ExceptionCodes::kTriggeredExceptionCode,
            L"TriggeredExceptionCode"s,
        },
    };

    for (const auto &testCase : testCases)
    {
        EXPECT_EQ(formatCommonException(testCase.input), testCase.output)
            << testCase.input;
    }
}
