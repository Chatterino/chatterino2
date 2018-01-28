#include "label.hpp"
#include "singletons/fontmanager.hpp"

#include <QPainter>

namespace chatterino {
namespace widgets {
Label::Label(BaseWidget *parent)
    : BaseWidget(parent)
{
    singletons::FontManager::getInstance().fontChanged.connect(
        [this]() { this->scaleChangedEvent(this->getScale()); });
}

const QString &Label::getText() const
{
    return this->text;
}

void Label::setText(const QString &value)
{
    this->text = value;
    this->scaleChangedEvent(this->getScale());
}

FontStyle Label::getFontStyle() const
{
    return this->fontStyle;
}

void Label::setFontStyle(FontStyle style)
{
    this->fontStyle = style;
    this->scaleChangedEvent(this->getScale());
}

void Label::scaleChangedEvent(float scale)
{
    QFontMetrics metrics =
        singletons::FontManager::getInstance().getFontMetrics(this->fontStyle, scale);

    this->preferedSize = QSize(metrics.width(this->text), metrics.height());

    this->updateGeometry();
}

QSize Label::sizeHint() const
{
    return this->preferedSize;
}

QSize Label::minimumSizeHint() const
{
    return this->preferedSize;
}

void Label::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setFont(singletons::FontManager::getInstance().getFont(
        this->fontStyle, this->getScale() / painter.device()->devicePixelRatioF()));

    int width = singletons::FontManager::getInstance()
                    .getFontMetrics(this->fontStyle, this->getScale())
                    .width(this->text);

    int flags = Qt::TextSingleLine;

    if (this->width() < width) {
        flags |= Qt::AlignLeft | Qt::AlignVCenter;
    } else {
        flags |= Qt::AlignCenter;
    }

    painter.drawText(this->rect(), flags, this->text);
}
}  // namespace widgets
}  // namespace chatterino
