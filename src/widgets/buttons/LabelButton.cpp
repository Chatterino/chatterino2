#include "widgets/buttons/LabelButton.hpp"

namespace chatterino {

LabelButton::LabelButton(const QString &text, BaseWidget *parent, QSize padding)
    : Button(parent)
    , layout_(this)
    , label_(text)
    , padding_(padding)
{
    this->layout_.setContentsMargins(0, 0, 0, 0);
    this->layout_.addWidget(&this->label_);
    this->label_.setAttribute(Qt::WA_TransparentForMouseEvents);
    this->label_.setAlignment(Qt::AlignCenter);

    this->updatePadding();
}

void LabelButton::setText(const QString &text)
{
    this->label_.setText(text);
}

QSize LabelButton::padding() const noexcept
{
    return this->padding_;
}

void LabelButton::setPadding(QSize padding)
{
    if (this->padding_ == padding)
    {
        return;
    }

    this->padding_ = padding;
    this->updatePadding();
}

void LabelButton::enableRichText()
{
    this->label_.setTextFormat(Qt::RichText);
}

void LabelButton::paintContent(QPainter &painter)
{
}

void LabelButton::updatePadding()
{
    auto x = this->padding_.width();
    auto y = this->padding_.height();
    this->label_.setContentsMargins(x, y, x, y);
}

}  // namespace chatterino
