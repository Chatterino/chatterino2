// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <pajlada/serialize.hpp>
#include <QStringView>

namespace pajlada {

template <>
struct Serialize<QStringView> {
    static rapidjson::Value get(const QStringView &value,
                                rapidjson::Document::AllocatorType &a)
    {
        return {value.toUtf8(), a};
    }
};

}  // namespace pajlada
