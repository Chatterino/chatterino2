#pragma once

#include "twitch-eventsub-ws/errors.hpp"

#define EVENTSUB_BAIL_HERE(kind)                                              \
    {                                                                         \
        static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION; \
        return ::chatterino::eventsub::lib::error::makeCode(kind, &loc);      \
    }
