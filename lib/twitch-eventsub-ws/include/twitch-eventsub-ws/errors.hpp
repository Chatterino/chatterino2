#pragma once

#include <boost/system/error_category.hpp>

#include <string>

namespace chatterino::eventsub::lib::error {

// NOLINTNEXTLINE(performance-enum-size)
enum class Kind : int {
    FieldMissing = 1,
    ExpectedObject,
    ExpectedString,
    UnknownEnumValue,
    InnerRootMissing,
    NoMessageHandler,
    UnknownVariant,
    BadTimeFormat,
};

class ApplicationErrorCategory final : public boost::system::error_category
{
public:
    consteval ApplicationErrorCategory() = default;

    const char *name() const noexcept override
    {
        return "JSON deserialization error";
    }

    std::string message(int ev) const override;
};

inline constexpr ApplicationErrorCategory CATEGORY;

inline boost::system::error_code makeCode(Kind kind,
                                          const boost::source_location *loc)
{
    return {static_cast<int>(kind), CATEGORY, loc};
}

}  // namespace chatterino::eventsub::lib::error
