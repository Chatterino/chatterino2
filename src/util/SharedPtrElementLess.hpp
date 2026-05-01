// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

namespace chatterino {

template <typename T>
struct SharedPtrElementLess {
    bool operator()(const std::shared_ptr<T> &a,
                    const std::shared_ptr<T> &b) const
    {
        return a->operator<(*b);
    }
};

}  // namespace chatterino
