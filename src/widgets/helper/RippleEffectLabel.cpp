#include "widgets/helper/RippleEffectLabel.hpp"
#include "singletons/Themes.hpp"
#include "widgets/splits/SplitHeader.hpp"

#include <QBrush>
#include <QPainter>

namespace chatterino {

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

RippleEffectLabel2::RippleEffectLabel2(BaseWidget *parent, int padding)
    : RippleEffectButton(parent)
    , label_(this)
{
    auto *hbox = new QHBoxLayout(this);
    this->setLayout(hbox);

    //    this->label_.setAlignment(Qt::AlignCenter);
    this->label_.setCentered(true);

    hbox->setMargin(0);
    //    hbox.addSpacing(spacing);
    hbox->addWidget(&this->label_);
    //    hbox.addSpacing(spacing);
}

Label &RippleEffectLabel2::getLabel()
{
    return this->label_;
}

}  // namespace chatterino
