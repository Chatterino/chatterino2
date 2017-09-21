#include "widgets/rippleeffectlabel.hpp"
#include "colorscheme.hpp"
#include "widgets/chatwidgetheader.hpp"

#include <QBrush>
#include <QPainter>

namespace chatterino {
namespace widgets {

RippleEffectLabel::RippleEffectLabel(BaseWidget *parent, int spacing)
    : RippleEffectButton(parent)
    , label(this)
{
    setLayout(&this->hbox);

    this->label.setAlignment(Qt::AlignCenter);

    this->hbox.setMargin(0);
    this->hbox.addSpacing(spacing);
    this->hbox.addWidget(&this->label);
    this->hbox.addSpacing(spacing);

    this->setMouseEffectColor(QColor(255, 255, 255, 63));
}

}  // namespace widgets
}  // namespace chatterino
