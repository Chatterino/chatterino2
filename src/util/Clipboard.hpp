// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

namespace chatterino {

void crossPlatformCopy(const QString &text);

QString getClipboardText();

}  // namespace chatterino
