#include "widgets/helper/EffectLabel.hpp"

#include <QBrush>
#include <QPainter>

namespace chatterino {

EffectLabel::EffectLabel(BaseWidget *parent, int spacing)
    : Button(parent)
    , label_(this)
{
    setLayout(&this->hbox_);

    this->label_.setAlignment(Qt::AlignCenter);

    this->hbox_.setContentsMargins(0, 0, 0, 0);
    this->hbox_.addSpacing(spacing);
    this->hbox_.addWidget(&this->label_);
    this->hbox_.addSpacing(spacing);
}

EffectLabel2::EffectLabel2(BaseWidget *parent, int padding)
    : Button(parent)
    , label_(this)
{
    auto *hbox = new QHBoxLayout(this);
    this->setLayout(hbox);

    //    this->label_.setAlignment(Qt::AlignCenter);
    this->label_.setCentered(true);

    hbox->setContentsMargins(0, 0, 0, 0);
    //    hbox.addSpacing(spacing);
    hbox->addWidget(&this->label_);
    //    hbox.addSpacing(spacing);
}

Label &EffectLabel2::getLabel()
{
    return this->label_;
}

}  // namespace chatterino
