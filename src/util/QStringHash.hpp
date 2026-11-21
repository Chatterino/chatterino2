// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <boost/container_hash/hash_fwd.hpp>
#include <QHash>
#include <QString>

namespace boost {

template <>
struct hash<QString> {
    std::size_t operator()(QString const &s) const
    {
        return qHash(s);
    }
};

}  // namespace boost
