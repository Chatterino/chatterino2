#ifndef CHATWIDGETHEADERBUTTON_H
#define CHATWIDGETHEADERBUTTON_H

#include "widgets/signallabel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {

class ChatWidgetHeaderButton : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidgetHeaderButton(int spacing = 6);

    SignalLabel &getLabel()
    {
        return _label;
    }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    QHBoxLayout _hbox;
    SignalLabel _label;

    bool _mouseOver;
    bool _mouseDown;

    void labelMouseUp();
    void labelMouseDown();
};
}
}

#endif  // CHATWIDGETHEADERBUTTON_H
