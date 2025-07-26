#include "widgets/buttons/DrawnButton.hpp"

#include "singletons/Theme.hpp"

#include <QPainter>

namespace chatterino {

DrawnButton::DrawnButton(Symbol symbol_, Options options, BaseWidget *parent)
    : Button(parent)
    , symbol(symbol_)
{
    this->setContentCacheEnabled(true);

    // Set user-defined options
    this->setOptions(options);

    // Ensure symbol-specific options are initially set
    this->themeChangedEvent();
}

void DrawnButton::setOptions(Options options_)
{
    this->options = options_;

    this->invalidateContent();
}

void DrawnButton::themeChangedEvent()
{
    Button::themeChangedEvent();

    auto &o = this->symbolOptions;

    switch (this->symbol)
    {
        case Symbol::Plus: {
            o.padding = 3;
            o.thickness = 1;

            o.foreground = this->theme->messages.textColors.system;
            o.foregroundHover = this->theme->messages.textColors.regular;
        }
        break;
    }

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
        bg = this->getBackgroundHover();
        fg = this->getForegroundHover();
    }
    else
    {
        bg = this->getBackground();
        fg = this->getForeground();
    }

    if (bg.isValid())
    {
        painter.fillRect(this->rect(), bg);
    }

    auto thickness = this->getThickness();
    auto padding = this->getPadding();

    switch (this->symbol)
    {
        case Symbol::Plus: {
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
    auto v =
        this->options.padding.value_or(this->symbolOptions.padding.value_or(0));

    return static_cast<int>(std::round(static_cast<float>(v) * this->scale()));
}

int DrawnButton::getThickness() const
{
    auto v = this->options.thickness.value_or(
        this->symbolOptions.thickness.value_or(1));

    return std::max(1, static_cast<int>(
                           std::round(static_cast<double>(v) * this->scale())));
}

QColor DrawnButton::getBackground() const
{
    auto v = this->options.background.value_or(
        this->symbolOptions.background.value_or(QColor()));

    return v;
}

QColor DrawnButton::getBackgroundHover() const
{
    auto v = this->options.backgroundHover.value_or(
        this->symbolOptions.backgroundHover.value_or(QColor()));

    return v;
}

QColor DrawnButton::getForeground() const
{
    auto v = this->options.foreground.value_or(
        this->symbolOptions.foreground.value_or(QColor()));

    assert(v.isValid());

    return v;
}

QColor DrawnButton::getForegroundHover() const
{
    auto v = this->options.foregroundHover.value_or(
        this->symbolOptions.foregroundHover.value_or(QColor()));

    assert(v.isValid());

    return v;
}

}  // namespace chatterino
