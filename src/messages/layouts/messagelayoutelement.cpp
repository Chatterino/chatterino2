#include "messages/layouts/messagelayoutelement.hpp"
#include "messages/messageelement.hpp"

#include <QPainter>

namespace chatterino {
namespace messages {
namespace layouts {

const QRect &MessageLayoutElement::getRect() const
{
    return this->rect;
}

MessageLayoutElement::MessageLayoutElement(MessageElement &_creator, const QSize &size)
    : creator(_creator)
{
    this->rect.setSize(size);
}

MessageElement &MessageLayoutElement::getCreator() const
{
    return this->creator;
}

void MessageLayoutElement::setPosition(QPoint point)
{
    this->rect.moveTopLeft(point);
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

//
// IMAGE
//

ImageLayoutElement::ImageLayoutElement(MessageElement &_creator, Image &_image, QSize _size)
    : MessageLayoutElement(_creator, _size)
    , image(_image)
{
    this->trailingSpace = _creator.hasTrailingSpace();
}

void ImageLayoutElement::addCopyTextToString(QString &str, int from, int to) const
{
    str += "<image>";
}

int ImageLayoutElement::getSelectionIndexCount()
{
    return this->trailingSpace ? 2 : 1;
}

void ImageLayoutElement::paint(QPainter &painter)
{
    const QPixmap *pixmap = this->image.getPixmap();

    if (pixmap != nullptr && !this->image.isAnimated()) {
        // fourtf: make it use qreal values
        painter.drawPixmap(QRectF(this->getRect()), *pixmap, QRectF());
    }
}

void ImageLayoutElement::paintAnimated(QPainter &painter, int yOffset)
{
    if (this->image.isAnimated()) {
        if (this->image.getPixmap() != nullptr) {
            // fourtf: make it use qreal values
            QRect rect = this->getRect();
            rect.moveTop(yOffset);
            painter.drawPixmap(QRectF(rect), *this->image.getPixmap(), QRectF());
        }
    }
}

int ImageLayoutElement::getMouseOverIndex(const QPoint &abs)
{
    return 0;
}

//
// TEXT
//

TextLayoutElement::TextLayoutElement(MessageElement &_creator, QString &_text, QSize _size,
                                     QColor _color, FontStyle _style, float _scale)
    : MessageLayoutElement(_creator, _size)
    , text(_text)
    , color(_color)
    , style(_style)
    , scale(_scale)
{
}

void TextLayoutElement::addCopyTextToString(QString &str, int from, int to) const
{
}

int TextLayoutElement::getSelectionIndexCount()
{
    return this->text.length() + (this->trailingSpace ? 1 : 0);
}

void TextLayoutElement::paint(QPainter &painter)
{
    painter.setPen(this->color);

    painter.setFont(singletons::FontManager::getInstance().getFont(this->style, this->scale));

    painter.drawText(QRectF(this->getRect().x(), this->getRect().y(), 10000, 10000), this->text,
                     QTextOption(Qt::AlignLeft | Qt::AlignTop));
}

void TextLayoutElement::paintAnimated(QPainter &, int)
{
}

int TextLayoutElement::getMouseOverIndex(const QPoint &abs)
{
    return 0;
}
}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
