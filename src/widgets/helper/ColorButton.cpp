#include "widgets/helper/ColorButton.hpp"

namespace chatterino {

ColorButton::ColorButton(const QColor &color)
    : color_(color)
{
    // TODO(leon): Replace magic numbers with constants
    this->setMinimumSize(32, 32);
    this->setMaximumSize(32, 32);
    this->setColor(color_);
}

const QColor &ColorButton::color() const
{
    return this->color_;
}

void ColorButton::setColor(QColor color)
{
    this->color_ = color;
    this->setStyleSheet("background-color: " + color.name(QColor::HexArgb));
}

}  // namespace chatterino
