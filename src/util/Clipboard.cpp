#include <QClipboard>
#include "util/Clipboard.hpp"

namespace chatterino {

void crossPlatformCopy(QClipboard *clipboard, const QString &text)
{
    clipboard->setText(text);
    if (clipboard->supportsSelection()) {
        clipboard->setText(text, QClipboard::Selection);
    }
}

}  // namespace chatterino
