#include "CmdDeleteKeyFilter.hpp"

#include <QTextCursor>
#include <QKeyEvent>
#include <QTextEdit>

CmdDeleteKeyFilter::CmdDeleteKeyFilter(QObject *parent)
    : QObject(parent)
{
}

bool CmdDeleteKeyFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (auto *textEdit = qobject_cast<QTextEdit *>(obj))
        {
#if defined(Q_OS_MACOS)
            if (keyEvent->modifiers() == Qt::ControlModifier &&
                keyEvent->key() == Qt::Key_Backspace)
            {
                QTextCursor cursor = textEdit->textCursor();
                if (!cursor.hasSelection())
                {
                    cursor.movePosition(QTextCursor::StartOfLine,
                                        QTextCursor::KeepAnchor);
                }
                cursor.removeSelectedText();
                textEdit->setTextCursor(cursor);
                return true;
            }
#endif
        }
    }
    return QObject::eventFilter(obj, event);
}
