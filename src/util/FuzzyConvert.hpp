// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

namespace chatterino {

int fuzzyToInt(const QString &str, int default_);
float fuzzyToFloat(const QString &str, float default_);

}  // namespace chatterino
