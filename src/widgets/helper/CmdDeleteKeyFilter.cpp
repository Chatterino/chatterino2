// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/CmdDeleteKeyFilter.hpp"

#include <QKeyEvent>
#include <QTextCursor>
#include <QTextEdit>

CmdDeleteKeyFilter::CmdDeleteKeyFilter(QObject *parent)
    : QObject(parent)
{
}

bool CmdDeleteKeyFilter::eventFilter(QObject *obj, QEvent *event)
{
#ifdef Q_OS_MACOS
    if (event->type() != QEvent::KeyPress)
    {
        return false;
    }

    auto *textEdit = qobject_cast<QTextEdit *>(obj);
    const auto *keyEvent = dynamic_cast<QKeyEvent *>(event);

    if (!textEdit || !keyEvent)
    {
        return false;
    }

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
    return false;
}
