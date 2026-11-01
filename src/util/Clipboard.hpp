// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

namespace chatterino {

/// Copy to the standard clipboard, copied from with ctrl+c and pasted from with ctrl+v.
/// If the Linux-specific "Primary"/"Selection" clipboard is available, also copy to it.
void crossPlatformCopy(const QString &text);

/// Copy to the "Primary"/"Selection" clipboard, only a thing on Linux.
/// This is copied to when selecting text, and in most applications can be pasted from with shift+insert or middle mouse button.
void copyToSelection(const QString &text);

QString getClipboardText();

}  // namespace chatterino
