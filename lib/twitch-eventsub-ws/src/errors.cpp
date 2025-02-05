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
        case Kind::FieldMissing: {
            auto msg = "Missing required key "s;
            msg += this->argument;
            return msg;
        }
        case Kind::ExpectedObject: {
            std::string msg(this->argument);
            msg += " must be an object";
            return msg;
        }
        case Kind::ExpectedString: {
            std::string msg(this->argument);
            msg += " must be a string";
            return msg;
        }
        case Kind::UnknownEnumValue: {
            auto msg = "No constant found to match "s;
            msg += this->argument;
            return msg;
        }
        case Kind::InnerRootMissing: {
            auto msg = "Missing inner root object "s;
            msg += this->argument;
            return msg;
        }
        case Kind::NoMessageHandler:
            return "No message handler found"s;
    }

    assert(false);
    return "Unknown error"s;
}

}  // namespace chatterino::eventsub::lib::error
