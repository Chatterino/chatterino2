#pragma once

#include <QFlags>
#include <QLabel>
#include <QMouseEvent>
#include <QWidget>

namespace chatterino {

class SignalLabel : public QLabel
{
    Q_OBJECT

public:
    explicit SignalLabel(QWidget *parent = nullptr, Qt::WindowFlags f = {});
    ~SignalLabel() override = default;

Q_SIGNALS:
    void mouseDoubleClick(QMouseEvent *ev);

    void leftMouseDown();
    void leftMouseUp();
    void mouseMove(QMouseEvent *event);

protected:
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

}  // namespace chatterino
