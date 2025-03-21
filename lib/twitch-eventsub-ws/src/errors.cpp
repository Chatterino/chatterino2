#include "twitch-eventsub-ws/errors.hpp"

#include <cassert>

namespace chatterino::eventsub::lib::error {

std::string ApplicationErrorCategory::message(int ev) const
{
    using namespace std::string_literals;

    static_assert(sizeof(int) == sizeof(Kind));
    auto kind = static_cast<Kind>(ev);

    switch (kind)
    {
        case Kind::FieldMissing:
            return "Missing required key"s;
        case Kind::ExpectedObject:
            return "Expected an object"s;
        case Kind::ExpectedString:
            return "Expected a string"s;
        case Kind::UnknownEnumValue:
            return "Unknown enum value"s;
        case Kind::InnerRootMissing:
            return "Missing inner root"s;
        case Kind::NoMessageHandler:
            return "No message handler found"s;
        case Kind::UnknownVariant:
            return "Unknown variant value"s;
        case Kind::BadTimeFormat:
            return "Bad time format"s;
    }

    assert(false);
    return "Unknown error"s;
}

}  // namespace chatterino::eventsub::lib::error
