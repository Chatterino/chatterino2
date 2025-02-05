#include "twitch-eventsub-ws/errors.hpp"

namespace chatterino::eventsub::lib::error::detail {

template <StaticString V>
inline constexpr ApplicationErrorCategory CATEGORY_ARG{V};

template <StaticString V>
inline boost::system::error_code fieldMissing()
{
    return {
        static_cast<int>(ApplicationErrorCategory::Kind::FieldMissing),
        CATEGORY_ARG<V>,
    };
}

template <StaticString V>
inline boost::system::error_code expectedObject()
{
    return {
        static_cast<int>(ApplicationErrorCategory::Kind::ExpectedObject),
        CATEGORY_ARG<V>,
    };
}

template <StaticString V>
inline boost::system::error_code expectedString()
{
    return {
        static_cast<int>(ApplicationErrorCategory::Kind::ExpectedString),
        CATEGORY_ARG<V>,
    };
}

template <StaticString V>
inline boost::system::error_code unknownEnumValue()
{
    return {
        static_cast<int>(ApplicationErrorCategory::Kind::UnknownEnumValue),
        CATEGORY_ARG<V>,
    };
}

template <StaticString V>
inline boost::system::error_code innerRootMissing()
{
    return {
        static_cast<int>(ApplicationErrorCategory::Kind::InnerRootMissing),
        CATEGORY_ARG<V>,
    };
}

inline boost::system::error_code noMessageHandler()
{
    return {
        static_cast<int>(ApplicationErrorCategory::Kind::NoMessageHandler),
        CATEGORY_ARG<"">,
    };
}

}  // namespace chatterino::eventsub::lib::error::detail
