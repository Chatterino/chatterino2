#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/rippleeffectbutton.hpp"
#include "widgets/signallabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {

class ColorScheme;

namespace widgets {

class RippleEffectLabel : public FancyButton
{
public:
    explicit RippleEffectLabel(BaseWidget *parent, int spacing = 6);

    SignalLabel &getLabel()
    {
        return this->ui.label;
    }

private:
    struct {
        QHBoxLayout hbox;
        SignalLabel label;
    } ui;
};

}  // namespace widgets
}  // namespace chatterino
