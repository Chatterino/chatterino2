#pragma once

#include <QObject>

/// CmdDeleteKeyFilter is an event filter for QTextEdit objects
///  to handle the behavior of the Command + Delete shortcut on macOS
class CmdDeleteKeyFilter : public QObject
{
    Q_OBJECT
public:
    explicit CmdDeleteKeyFilter(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};
