#pragma once

#include <boost/system/error_category.hpp>

#include <string>

namespace chatterino::eventsub::lib::error::detail {

template <size_t N>
struct StaticString {
    static_assert(N > 0);

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    consteval StaticString(const char (&arr)[N])
    {
        for (size_t i = 0; i < N; i++)
        {
            data[i] = arr[i];
        }
    }

    consteval operator std::string_view() const
    {
        return {this->data, N - 1};
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    char data[N]{};
};

}  // namespace chatterino::eventsub::lib::error::detail

namespace chatterino::eventsub::lib::error {

/// boost::system::error_code doesn't support string arguments, so we're using
/// the error_category to store strings. All strings are statically allocated.
class ApplicationErrorCategory final : public boost::system::error_category
{
    std::string_view argument;

public:
    // NOLINTNEXTLINE(performance-enum-size)
    enum class Kind : int {
        FieldMissing = 1,
        ExpectedObject,
        ExpectedString,
        UnknownEnumValue,
        InnerRootMissing,
        NoMessageHandler
    };

    consteval ApplicationErrorCategory(std::string_view argument)
        : argument(argument)
    {
    }

    const char *name() const noexcept override
    {
        return "JSON deserialization error";
    }

    std::string message(int ev) const override;
};

}  // namespace chatterino::eventsub::lib::error
