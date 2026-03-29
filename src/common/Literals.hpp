// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

/// Re-export of Qt's string literals. In new code, prefer using
/// Qt::StringLiterals.
namespace chatterino::literals {

using Qt::StringLiterals::operator""_s;
using Qt::StringLiterals::operator""_ba;
using Qt::StringLiterals::operator""_L1;

}  // namespace chatterino::literals
