#include "util/Clipboard.hpp"

#include <QApplication>
#include <QClipboard>

namespace chatterino {

void crossPlatformCopy(const QString &text)
{
    auto *clipboard = QApplication::clipboard();

    clipboard->setText(text);

    if (clipboard->supportsSelection())
    {
        clipboard->setText(text, QClipboard::Selection);
    }
}

QString getClipboardText()
{
    return QApplication::clipboard()->text();
}

}  // namespace chatterino
