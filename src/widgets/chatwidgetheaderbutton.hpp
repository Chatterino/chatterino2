#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/fancybutton.hpp"
#include "widgets/signallabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {

class ColorScheme;

namespace widgets {

class ChatWidgetHeader;

class ChatWidgetHeaderButton : public FancyButton
{
public:
    explicit ChatWidgetHeaderButton(BaseWidget *parent, int spacing = 6);

    SignalLabel &getLabel()
    {
        return this->ui.label;
    }

protected:
    //    virtual void paintEvent(QPaintEvent *) override;

private:
    struct {
        QHBoxLayout hbox;
        SignalLabel label;
    } ui;
};

}  // namespace widgets
}  // namespace chatterino
