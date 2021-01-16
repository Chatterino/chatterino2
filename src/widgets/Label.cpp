#include "Label.hpp"

#include <QPainter>

namespace chatterino {

Label::Label(QString text, FontStyle style)
    : Label(nullptr, text, style)
{
}

Label::Label(BaseWidget *parent, QString text, FontStyle style)
    : BaseWidget(parent)
    , text_(text)
    , fontStyle_(style)
{
    this->connections_.managedConnect(getFonts()->fontChanged, [this] {
        this->updateSize();
    });
}

const QString &Label::getText() const
{
    return this->text_;
}

void Label::setText(const QString &text)
{
    if (this->text_ != text)
    {
        this->text_ = text;
        this->updateSize();
        this->update();
    }
}

FontStyle Label::getFontStyle() const
{
    return this->fontStyle_;
}

bool Label::getCentered() const
{
    return this->centered_;
}

void Label::setCentered(bool centered)
{
    this->centered_ = centered;
    this->updateSize();
}

bool Label::getHasOffset() const
{
    return this->hasOffset_;
}

void Label::setHasOffset(bool hasOffset)
{
    this->hasOffset_ = hasOffset;
    this->updateSize();
}
void Label::setFontStyle(FontStyle style)
{
    this->fontStyle_ = style;
    this->updateSize();
}

void Label::scaleChangedEvent(float scale)
{
    this->updateSize();
}

QSize Label::sizeHint() const
{
    return this->preferedSize_;
}

QSize Label::minimumSizeHint() const
{
    return this->preferedSize_;
}

void Label::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    qreal deviceDpi =
#ifdef Q_OS_WIN
        this->devicePixelRatioF();
#else
        1.0;
#endif

    QFontMetrics metrics = getFonts()->getFontMetrics(
        this->getFontStyle(),
        this->scale() * 96.f /
            std::max<float>(0.01, this->logicalDpiX() * deviceDpi));
    painter.setFont(getFonts()->getFont(
        this->getFontStyle(),
        this->scale() * 96.f /
            std::max<float>(0.02, this->logicalDpiX() * deviceDpi)));

    int offset = this->getOffset();

    // draw text
    QRect textRect(offset, 0, this->width() - offset - offset, this->height());

    int width = metrics.width(this->text_);
    Qt::Alignment alignment = !this->centered_ || width > textRect.width()
                                  ? Qt::AlignLeft | Qt::AlignVCenter
                                  : Qt::AlignCenter;

    painter.setBrush(this->palette().windowText());

    QTextOption option(alignment);
    option.setWrapMode(QTextOption::NoWrap);
    painter.drawText(textRect, this->text_, option);

#if 0
    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
#endif
}

void Label::updateSize()
{
    QFontMetrics metrics =
        getFonts()->getFontMetrics(this->fontStyle_, this->scale());

    int width = metrics.width(this->text_) + (2 * this->getOffset());
    int height = metrics.height();
    this->preferedSize_ = QSize(width, height);

    this->updateGeometry();
}

int Label::getOffset()
{
    return this->hasOffset_ ? int(8 * this->scale()) : 0;
}

}  // namespace chatterino
