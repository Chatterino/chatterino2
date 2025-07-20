#include "widgets/buttons/DrawnButton.hpp"

#include "singletons/Theme.hpp"

#include <QPainter>

namespace chatterino {

DrawnButton::DrawnButton(Type type_, Options options, BaseWidget *parent)
    : Button(parent)
    , foreground(this->theme->messages.textColors.system)
    , foregroundHover(this->theme->messages.textColors.regular)
    , basePadding(options.padding)
    , baseThickness(options.thickness)
    , type(type_)
{
    this->setContentCacheEnabled(true);
}

void DrawnButton::setBackground(QColor color)
{
    this->background = color;
    this->invalidateContent();
}

void DrawnButton::setBackgroundHover(QColor color)
{
    this->backgroundHover = color;
    this->invalidateContent();
}

void DrawnButton::setForeground(QColor color)
{
    this->foreground = color;
    this->invalidateContent();
}

void DrawnButton::setForegroundHover(QColor color)
{
    this->foregroundHover = color;
    this->invalidateContent();
}

void DrawnButton::mouseOverUpdated()
{
    this->invalidateContent();
}

void DrawnButton::paintContent(QPainter &painter)
{
    QColor fg;
    QColor bg;

    if (this->mouseOver())
    {
        bg = this->backgroundHover;
        fg = this->foregroundHover;
    }
    else
    {
        bg = this->background;
        fg = this->foreground;
    }

    if (bg.isValid())
    {
        painter.fillRect(this->rect(), bg);
    }

    auto thickness = this->getThickness();
    auto padding = this->getPadding();

    switch (this->type)
    {
        case Type::Plus: {
            QPen pen;
            pen.setColor(fg);
            pen.setWidth(thickness);
            painter.setPen(pen);

            auto innerSize = this->rect().size();
            innerSize.setHeight(innerSize.width());
            QRect inner;
            inner.setSize(innerSize);
            inner.moveCenter(this->rect().center());

            auto top = inner.top();
            auto bottom = inner.bottom();
            auto center = inner.center();
            auto left = inner.left();
            auto right = inner.right();

            QLine vertical(center.x(), top + padding, center.x(),
                           bottom - padding);
            painter.drawLine(vertical);
            QLine horizontal(left + padding, center.y(), right - padding,
                             center.y());
            painter.drawLine(horizontal);
        }
        break;
    }
}

int DrawnButton::getPadding() const
{
    return static_cast<int>(static_cast<float>(this->basePadding) *
                            this->scale());
}

int DrawnButton::getThickness() const
{
    return std::max(
        1, static_cast<int>(std::round(
               static_cast<double>(this->baseThickness) * this->scale())));
}

}  // namespace chatterino
