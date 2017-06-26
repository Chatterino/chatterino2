#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/signallabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {

class ColorScheme;

namespace widgets {

class ChatWidgetHeader;

class ChatWidgetHeaderButton : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChatWidgetHeaderButton(BaseWidget *parent, int spacing = 6);

    SignalLabel &getLabel()
    {
        return this->ui.label;
    }

signals:
    void clicked();

protected:
    virtual void paintEvent(QPaintEvent *) override;

    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct {
        QHBoxLayout hbox;
        SignalLabel label;
    } ui;

    bool mouseOver = false;
    bool mouseDown = false;

    void labelMouseUp();
    void labelMouseDown();
};

}  // namespace widgets
}  // namespace chatterino
