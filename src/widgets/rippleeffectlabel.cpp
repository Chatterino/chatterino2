#include "widgets/rippleeffectlabel.hpp"
#include "colorscheme.hpp"
#include "widgets/chatwidgetheader.hpp"

#include <QBrush>
#include <QPainter>

namespace chatterino {
namespace widgets {

RippleEffectLabel::RippleEffectLabel(BaseWidget *parent, int spacing)
    : RippleEffectButton(parent)
{
    setLayout(&this->ui.hbox);

    this->ui.label.setAlignment(Qt::AlignCenter);

    this->ui.hbox.setMargin(0);
    this->ui.hbox.addSpacing(spacing);
    this->ui.hbox.addWidget(&this->ui.label);
    this->ui.hbox.addSpacing(spacing);

    this->setMouseEffectColor(QColor(255, 255, 255, 63));
}

}  // namespace widgets
}  // namespace chatterino
