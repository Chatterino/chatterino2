// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <QJsonValue>

template <>
struct magic_enum::customize::enum_range<QJsonValue::Type> {
    static constexpr int max = QJsonValue::Type::Undefined;  // NOLINT
};
