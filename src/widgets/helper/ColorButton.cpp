#include "widgets/helper/ColorButton.hpp"

namespace chatterino {

ColorButton::ColorButton(const QColor &color, QWidget *parent)
    : QPushButton(parent)
    , color_(color)
{
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
