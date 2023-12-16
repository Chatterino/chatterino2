#include "WinSupport.hpp"

#include <util/win/exception_codes.h>
#include <Windows.h>

#define MAGIC_ENUM_USING_ALIAS_STRING_VIEW \
    using string_view = std::wstring_view;
#define MAGIC_ENUM_USING_ALIAS_STRING using string = std::wstring;
#include <magic_enum/magic_enum.hpp>

using namespace std::string_literals;

namespace {

enum class WinException : uint32_t {
    ExceptionAccessViolation = EXCEPTION_ACCESS_VIOLATION,
    ExceptionDatatypeMisalignment = EXCEPTION_DATATYPE_MISALIGNMENT,
    ExceptionBreakpoint = EXCEPTION_BREAKPOINT,
    ExceptionSingleStep = EXCEPTION_SINGLE_STEP,
    ExceptionArrayBoundsExceeded = EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
    ExceptionFltDenormalOperand = EXCEPTION_FLT_DENORMAL_OPERAND,
    ExceptionFltDivideByZero = EXCEPTION_FLT_DIVIDE_BY_ZERO,
    ExceptionFltInexactResult = EXCEPTION_FLT_INEXACT_RESULT,
    ExceptionFltInvalidOperation = EXCEPTION_FLT_INVALID_OPERATION,
    ExceptionFltOverflow = EXCEPTION_FLT_OVERFLOW,
    ExceptionFltStackCheck = EXCEPTION_FLT_STACK_CHECK,
    ExceptionFltUnderflow = EXCEPTION_FLT_UNDERFLOW,
    ExceptionIntDivideByZero = EXCEPTION_INT_DIVIDE_BY_ZERO,
    ExceptionIntOverflow = EXCEPTION_INT_OVERFLOW,
    ExceptionPrivInstruction = EXCEPTION_PRIV_INSTRUCTION,
    ExceptionInPageError = EXCEPTION_IN_PAGE_ERROR,
    ExceptionIllegalInstruction = EXCEPTION_ILLEGAL_INSTRUCTION,
    ExceptionNoncontinuableException = EXCEPTION_NONCONTINUABLE_EXCEPTION,
    ExceptionStackOverflow = EXCEPTION_STACK_OVERFLOW,
    ExceptionInvalidDisposition = EXCEPTION_INVALID_DISPOSITION,
    ExceptionGuardPage = EXCEPTION_GUARD_PAGE,
    ExceptionInvalidHandle = EXCEPTION_INVALID_HANDLE,
};

}  // namespace

template <>
struct magic_enum::customize::enum_range<WinException> {
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr uint32_t min = 0xC0000000L;
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr uint32_t max = 0xC00000ffL;
};

std::optional<std::wstring> formatCommonException(uint32_t ex)
{
    if (ex == crashpad::ExceptionCodes::kTriggeredExceptionCode)
    {
        return L"TriggeredExceptionCode"s;
    }

    auto name = magic_enum::enum_name(static_cast<WinException>(ex));
    if (name.empty())
    {
        return std::nullopt;
    }
    return std::wstring{name};
}
