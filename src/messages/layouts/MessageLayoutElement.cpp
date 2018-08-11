#include "messages/layouts/MessageLayoutElement.hpp"

#include "Application.hpp"
#include "messages/MessageElement.hpp"
#include "util/DebugCount.hpp"

#include <QDebug>
#include <QPainter>

namespace chatterino {

const QRect &MessageLayoutElement::getRect() const
{
    return this->rect_;
}

MessageLayoutElement::MessageLayoutElement(MessageElement &creator,
                                           const QSize &size)
    : creator_(creator)
{
    this->rect_.setSize(size);
    DebugCount::increase("message layout elements");
}

MessageLayoutElement::~MessageLayoutElement()
{
    DebugCount::decrease("message layout elements");
}

MessageElement &MessageLayoutElement::getCreator() const
{
    return this->creator_;
}

void MessageLayoutElement::setPosition(QPoint point)
{
    this->rect_.moveTopLeft(point);
}

bool MessageLayoutElement::hasTrailingSpace() const
{
    return this->trailingSpace;
}

MessageLayoutElement *MessageLayoutElement::setTrailingSpace(bool value)
{
    this->trailingSpace = value;

    return this;
}

MessageLayoutElement *MessageLayoutElement::setLink(const Link &_link)
{
    this->link_ = _link;
    return this;
}

const Link &MessageLayoutElement::getLink() const
{
    return this->link_;
}

//
// IMAGE
//

ImageLayoutElement::ImageLayoutElement(MessageElement &creator, ImagePtr image,
                                       const QSize &size)
    : MessageLayoutElement(creator, size)
    , image_(image)
{
    this->trailingSpace = creator.hasTrailingSpace();
}

void ImageLayoutElement::addCopyTextToString(QString &str, int from,
                                             int to) const
{
    //    str += this->image_->getCopyString();
    str += "not implemented";

    if (this->hasTrailingSpace()) {
        str += " ";
    }
}

int ImageLayoutElement::getSelectionIndexCount()
{
    return this->trailingSpace ? 2 : 1;
}

void ImageLayoutElement::paint(QPainter &painter)
{
    if (this->image_ == nullptr) {
        return;
    }

    auto pixmap = this->image_->pixmap();
    if (pixmap && !this->image_->animated()) {
        // fourtf: make it use qreal values
        painter.drawPixmap(QRectF(this->getRect()), *pixmap, QRectF());
    }
}

void ImageLayoutElement::paintAnimated(QPainter &painter, int yOffset)
{
    if (this->image_ == nullptr) {
        return;
    }

    if (this->image_->animated()) {
        if (auto pixmap = this->image_->pixmap()) {
            auto rect = this->getRect();
            rect.moveTop(rect.y() + yOffset);
            painter.drawPixmap(QRectF(rect), *pixmap, QRectF());
        }
    }
}

int ImageLayoutElement::getMouseOverIndex(const QPoint &abs)
{
    return 0;
}

int ImageLayoutElement::getXFromIndex(int index)
{
    if (index <= 0) {
        return this->getRect().left();
    } else if (index == 1) {
        // fourtf: remove space width
        return this->getRect().right();
    } else {
        return this->getRect().right();
    }
}

//
// TEXT
//

TextLayoutElement::TextLayoutElement(MessageElement &_creator, QString &_text,
                                     const QSize &_size, QColor _color,
                                     FontStyle _style, float _scale)
    : MessageLayoutElement(_creator, _size)
    , text(_text)
    , color(_color)
    , style(_style)
    , scale(_scale)
{
}

void TextLayoutElement::addCopyTextToString(QString &str, int from,
                                            int to) const
{
    str += this->text.mid(from, to - from);

    if (this->hasTrailingSpace()) {
        str += " ";
    }
}

int TextLayoutElement::getSelectionIndexCount()
{
    return this->text.length() + (this->trailingSpace ? 1 : 0);
}

void TextLayoutElement::paint(QPainter &painter)
{
    auto app = getApp();

    painter.setPen(this->color);

    painter.setFont(app->fonts->getFont(this->style, this->scale));

    painter.drawText(
        QRectF(this->getRect().x(), this->getRect().y(), 10000, 10000),
        this->text, QTextOption(Qt::AlignLeft | Qt::AlignTop));
}

void TextLayoutElement::paintAnimated(QPainter &, int)
{
}

int TextLayoutElement::getMouseOverIndex(const QPoint &abs)
{
    if (abs.x() < this->getRect().left()) {
        return 0;
    }

    auto app = getApp();

    QFontMetrics metrics = app->fonts->getFontMetrics(this->style, this->scale);

    int x = this->getRect().left();

    for (int i = 0; i < this->text.size(); i++) {
        int w = metrics.width(this->text[i]);

        if (x + w > abs.x()) {
            return i;
        }

        x += w;
    }

    return this->getSelectionIndexCount();
}

int TextLayoutElement::getXFromIndex(int index)
{
    auto app = getApp();

    QFontMetrics metrics = app->fonts->getFontMetrics(this->style, this->scale);

    if (index <= 0) {
        return this->getRect().left();
    } else if (index < this->text.size()) {
        int x = 0;
        for (int i = 0; i < index; i++) {
            x += metrics.width(this->text[i]);
        }
        return x + this->getRect().left();
    } else {
        return this->getRect().right();
    }
}

// TEXT ICON
TextIconLayoutElement::TextIconLayoutElement(MessageElement &creator,
                                             const QString &_line1,
                                             const QString &_line2,
                                             float _scale, const QSize &size)
    : MessageLayoutElement(creator, size)
    , scale(_scale)
    , line1(_line1)
    , line2(_line2)
{
}

void TextIconLayoutElement::addCopyTextToString(QString &str, int from,
                                                int to) const
{
}

int TextIconLayoutElement::getSelectionIndexCount()
{
    return this->trailingSpace ? 2 : 1;
}

void TextIconLayoutElement::paint(QPainter &painter)
{
    auto app = getApp();

    QFont font = app->fonts->getFont(FontStyle::Tiny, this->scale);

    painter.setPen(app->themes->messages.textColors.system);
    painter.setFont(font);

    QTextOption option;
    option.setAlignment(Qt::AlignHCenter);

    if (this->line2.isEmpty()) {
        QRect _rect(this->getRect());
        painter.drawText(_rect, this->line1, option);
    } else {
        painter.drawText(
            QPoint(this->getRect().x(),
                   this->getRect().y() + this->getRect().height() / 2),
            this->line1);
        painter.drawText(QPoint(this->getRect().x(),
                                this->getRect().y() + this->getRect().height()),
                         this->line2);
    }
}

void TextIconLayoutElement::paintAnimated(QPainter &painter, int yOffset)
{
}

int TextIconLayoutElement::getMouseOverIndex(const QPoint &abs)
{
    return 0;
}

int TextIconLayoutElement::getXFromIndex(int index)
{
    if (index <= 0) {
        return this->getRect().left();
    } else if (index == 1) {
        // fourtf: remove space width
        return this->getRect().right();
    } else {
        return this->getRect().right();
    }
}

}  // namespace chatterino
