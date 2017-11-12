#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/helper/rippleeffectbutton.hpp"
#include "widgets/helper/signallabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {

class ColorScheme;

namespace widgets {

class RippleEffectLabel : public RippleEffectButton
{
public:
    explicit RippleEffectLabel(BaseWidget *parent, int spacing = 6);

    SignalLabel &getLabel()
    {
        return this->label;
    }

private:
    QHBoxLayout hbox;
    SignalLabel label;
};

}  // namespace widgets
}  // namespace chatterino
