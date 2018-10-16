#pragma once

#include <algorithm>

namespace chatterino {
namespace util {

template <typename Container, typename UnaryPredicate>
typename Container::iterator find_if(Container &container, UnaryPredicate pred)
{
    return std::find_if(container.begin(), container.end(), pred);
}

template <typename Container, typename UnaryPredicate>
bool any_of(Container &container, UnaryPredicate pred)
{
    return std::any_of(container.begin(), container.end(), pred);
}

}  // namespace util
}  // namespace chatterino
