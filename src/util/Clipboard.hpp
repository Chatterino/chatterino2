#pragma once

#include <QString>

namespace chatterino {

void crossPlatformCopy(const QString &text);

QString getClipboardText();

}  // namespace chatterino
