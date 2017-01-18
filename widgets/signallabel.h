#ifndef SIGNALLABEL_H
#define SIGNALLABEL_H

#include <QFlags>
#include <QLabel>
#include <QWidget>

class SignalLabel : public QLabel
{
    Q_OBJECT
public:
    explicit SignalLabel(QWidget *parent = 0, Qt::WindowFlags f = 0)
        : QLabel(parent, f) {}
    virtual ~SignalLabel() = default;
signals:
    void mouseDoubleClick(QMouseEvent *ev);
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *ev) override {
        emit this->mouseDoubleClick(ev);
    }
};

#endif // SIGNALLABEL_H
