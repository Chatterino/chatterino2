#pragma once

class QClipboard;

namespace chatterino {

void crossPlatformCopy(QClipboard *clipboard, const QString &text);

}  // namespace chatterino
