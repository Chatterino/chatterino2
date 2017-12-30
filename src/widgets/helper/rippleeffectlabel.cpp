#include "widgets/helper/rippleeffectlabel.hpp"
#include "singletons/thememanager.hpp"
#include "widgets/helper/splitheader.hpp"

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
}

}  // namespace widgets
}  // namespace chatterino
