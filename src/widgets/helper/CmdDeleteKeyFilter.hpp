#pragma once

#include <QObject>

class CmdDeleteKeyFilter : public QObject
{
    Q_OBJECT
public:
    explicit CmdDeleteKeyFilter(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};
