#pragma once

#include <boost/json/object.hpp>

#include <iostream>
#include <optional>
#include <string_view>

namespace eventsub {

template <typename T>
std::optional<T> readMember(const boost::json::object &obj,
                            std::string_view key)
{
    const auto *it = obj.find(key);

    if (it == obj.end())
    {
        // No member with the key found
        std::cerr << "No member with the key " << key << " found\n";
        return std::nullopt;
    }

    const auto result = boost::json::try_value_to<T>(it->value());
    if (!result.has_value())
    {
        std::cerr << key << " could not be deserialized to the desired type\n";
        // Member could not be serialized to this type
        return std::nullopt;
    }

    return result.value();
}

}  // namespace eventsub
