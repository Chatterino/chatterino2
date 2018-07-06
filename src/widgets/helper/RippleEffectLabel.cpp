#include "widgets/helper/RippleEffectLabel.hpp"
#include "singletons/Theme.hpp"
#include "widgets/splits/SplitHeader.hpp"

#include <QBrush>
#include <QPainter>

namespace chatterino {

RippleEffectLabel::RippleEffectLabel(BaseWidget *parent, int spacing)
    : RippleEffectButton(parent)
    , label_(this)
{
    setLayout(&this->hbox_);

    this->label_.setAlignment(Qt::AlignCenter);

    this->hbox_.setMargin(0);
    this->hbox_.addSpacing(spacing);
    this->hbox_.addWidget(&this->label_);
    this->hbox_.addSpacing(spacing);
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
