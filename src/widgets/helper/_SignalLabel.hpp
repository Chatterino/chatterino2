#pragma once

#include <QFlags>
#include <QLabel>
#include <QMouseEvent>
#include <QWidget>

class SignalLabel : public QLabel
{
    Q_OBJECT

public:
    explicit SignalLabel(QWidget *parent = nullptr, Qt::WindowFlags f = 0);
    virtual ~SignalLabel() override = default;

signals:
    void mouseDoubleClick(QMouseEvent *ev);

    void mouseDown();
    void mouseUp();
    void mouseMove(QMouseEvent *event);

protected:
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};
