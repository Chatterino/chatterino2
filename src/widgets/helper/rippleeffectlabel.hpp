#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/helper/rippleeffectbutton.hpp"
#include "widgets/helper/signallabel.hpp"
#include "widgets/label.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {

class RippleEffectLabel : public RippleEffectButton
{
public:
    explicit RippleEffectLabel(BaseWidget *parent = nullptr, int spacing = 6);

    SignalLabel &getLabel()
    {
        return this->label;
    }

private:
    QHBoxLayout hbox;
    SignalLabel label;
};

class RippleEffectLabel2 : public RippleEffectButton
{
public:
    explicit RippleEffectLabel2(BaseWidget *parent = nullptr, int padding = 6);

    Label &getLabel();

private:
    Label label_;
};

}  // namespace widgets
}  // namespace chatterino
