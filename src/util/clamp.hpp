#pragma once

#include <algorithm>

namespace chatterino {
namespace util {

// http://en.cppreference.com/w/cpp/algorithm/clamp

template <class T>
constexpr const T &clamp(const T &v, const T &lo, const T &hi)
{
    return clamp(v, lo, hi, std::less<>());
}

template <class T, class Compare>
constexpr const T &clamp(const T &v, const T &lo, const T &hi, Compare comp)
{
    return assert(!comp(hi, lo)), comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

}  // namespace util
}  // namespace chatterino
