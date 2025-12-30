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
            o.padding = 4;
            o.thickness = 1;

            o.foreground = this->theme->messages.textColors.system;
            o.foregroundHover = this->theme->messages.textColors.regular;
        }
        break;

        case Symbol::Kebab: {
            o.padding = 2;
            o.thickness = 2;

            if (this->theme->isLightTheme())
            {
                // TODO: This should use its own theme color (e.g. theme->button->regular)
                o.foreground = QColor("#424242");
            }
            else
            {
                // TODO: This should use its own theme color (e.g. theme->button->regular)
                o.foreground = QColor("#c0c0c0");
            }
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
            inner = inner.marginsRemoved({padding, padding, padding, padding});

            // make sure that the inner size is always odd:
            // (width): [====outer====][|inner|][====outer====]
            // -> 2 * outer_w + inner_w
            // -> 2 * outer_w + 1dp
            // -> odd
            if ((inner.width() % 2) == 0)
            {
                inner.setRight(inner.right() + 1);
            }
            if ((inner.height() % 2) == 0)
            {
                inner.setTop(inner.top() - 1);
            }

            auto top = inner.top();
            auto bottom = inner.bottom();
            auto center = inner.center();
            auto left = inner.left();
            auto right = inner.right();

            QLine vertical(center.x(), top, center.x(), bottom);
            painter.drawLine(vertical);
            QLine horizontal(left, center.y(), right, center.y());
            painter.drawLine(horizontal);
        }
        break;

        case Symbol::Kebab: {
            QPen pen;
            pen.setColor(fg);
            pen.setWidth(thickness);
            painter.setPen(pen);

            QRect centerBox;
            centerBox.setSize({thickness, thickness});
            centerBox.moveCenter(this->rect().center());

            painter.fillRect(centerBox, fg);

            // NOTE: Technically a misuse of padding
            auto bottomBox = centerBox.translated(0, thickness + padding);
            painter.fillRect(bottomBox, fg);

            // NOTE: Technically a misuse of padding
            auto topBox = centerBox.translated(0, -(thickness + padding));
            painter.fillRect(topBox, fg);
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
