#include "widgets/Label.hpp"

#include "Application.hpp"

#include <QPainter>

namespace chatterino {

Label::Label(QString text, FontStyle style)
    : Label(nullptr, std::move(text), style)
{
}

Label::Label(BaseWidget *parent, QString text, FontStyle style)
    : BaseWidget(parent)
    , text_(std::move(text))
    , fontStyle_(style)
{
    this->connections_.managedConnect(getApp()->getFonts()->fontChanged,
                                      [this] {
                                          this->updateSize();
                                      });
    this->updateSize();
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

bool Label::getWordWrap() const
{
    return this->wordWrap_;
}

void Label::setWordWrap(bool wrap)
{
    this->wordWrap_ = wrap;
    this->update();
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

    QFontMetrics metrics = getApp()->getFonts()->getFontMetrics(
        this->getFontStyle(), this->scale());
    painter.setFont(
        getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));

    int offset = this->getOffset();

    // draw text
    QRect textRect(offset, 0, this->width() - offset - offset, this->height());

    int width = metrics.horizontalAdvance(this->text_);
    Qt::Alignment alignment = !this->centered_ || width > textRect.width()
                                  ? Qt::AlignLeft | Qt::AlignVCenter
                                  : Qt::AlignCenter;

    painter.setBrush(this->palette().windowText());

    QTextOption option(alignment);
    if (this->wordWrap_)
    {
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    }
    else
    {
        option.setWrapMode(QTextOption::NoWrap);
    }
    painter.drawText(textRect, this->text_, option);

#if 0
    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
#endif
}

void Label::updateSize()
{
    QFontMetrics metrics =
        getApp()->getFonts()->getFontMetrics(this->fontStyle_, this->scale());

    int width =
        metrics.horizontalAdvance(this->text_) + (2 * this->getOffset());
    int height = metrics.height();
    this->preferedSize_ = QSize(width, height);

    this->updateGeometry();
}

int Label::getOffset()
{
    return this->hasOffset_ ? int(8 * this->scale()) : 0;
}

}  // namespace chatterino
