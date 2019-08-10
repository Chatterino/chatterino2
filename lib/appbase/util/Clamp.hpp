#pragma once

namespace AB_NAMESPACE {

// http://en.cppreference.com/w/cpp/algorithm/clamp

template <class T>
constexpr const T &clamp(const T &v, const T &lo, const T &hi)
{
    return assert(!(hi < lo)), (v < lo) ? lo : (hi < v) ? hi : v;
}

}  // namespace AB_NAMESPACE
