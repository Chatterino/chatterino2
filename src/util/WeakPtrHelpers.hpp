// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

namespace chatterino {

/// Check if the owner of two weak pointers is equal.
///
/// Like `std::weak_ptr::owner_before`, this compares the control blocks,
/// not the objects themselves.
/// Equivalent to the C++ 26 `std::weak_ptr::owner_equal`.
template <typename T>
bool weakOwnerEquals(const std::weak_ptr<T> &a,
                     const std::weak_ptr<T> &b) noexcept
{
    return !a.owner_before(b) && !b.owner_before(a);
}

}  // namespace chatterino
