#include "messagelayoutcontainer.hpp"

#include "messagelayoutelement.hpp"
#include "messages/selection.hpp"
#include "singletons/settingsmanager.hpp"

#include <QDebug>
#include <QPainter>

#define COMPACT_EMOTES_OFFSET 6

namespace chatterino {
namespace messages {
namespace layouts {
MessageLayoutContainer::MessageLayoutContainer()
    : scale(1)
    , margin(4, 8, 4, 8)
    , centered(false)
{
    this->clear();
}

int MessageLayoutContainer::getHeight() const
{
    return this->height;
}

// methods
void MessageLayoutContainer::clear()
{
    this->elements.clear();

    this->height = 0;
    this->line = 0;
    this->currentX = 0;
    this->currentY = 0;
    this->lineStart = 0;
    this->lineHeight = 0;
}

void MessageLayoutContainer::addElement(MessageLayoutElement *element)
{
    if (!this->fitsInLine(element->getRect().width())) {
        this->breakLine();
    }

    this->_addElement(element);
}

void MessageLayoutContainer::addElementNoLineBreak(MessageLayoutElement *element)
{
    this->_addElement(element);
}

void MessageLayoutContainer::_addElement(MessageLayoutElement *element)
{
    if (this->elements.size() == 0) {
        this->currentY = this->margin.top * this->scale;
    }

    int newLineHeight = element->getRect().height();

    // fourtf: xD
    //    bool compactEmotes = true;
    //    if (compactEmotes && element->word.isImage() && word.getFlags() &
    //    MessageElement::EmoteImages) {
    //        newLineHeight -= COMPACT_EMOTES_OFFSET * this->scale;
    //    }

    this->lineHeight = std::max(this->lineHeight, newLineHeight);

    element->setPosition(QPoint(this->currentX, this->currentY - element->getRect().height()));
    this->elements.push_back(std::unique_ptr<MessageLayoutElement>(element));

    this->currentX += element->getRect().width();

    if (element->hasTrailingSpace()) {
        this->currentX += this->spaceWidth;
    }
}

void MessageLayoutContainer::breakLine()
{
    int xOffset = 0;

    if (this->centered && this->elements.size() > 0) {
        xOffset = (width - this->elements.at(this->elements.size() - 1)->getRect().right()) / 2;
    }

    for (size_t i = lineStart; i < this->elements.size(); i++) {
        MessageLayoutElement *element = this->elements.at(i).get();

        bool isCompactEmote = false;

        // fourtf: xD
        // this->enableCompactEmotes && element->getWord().isImage() &&
        //                     element->getWord().getFlags() &
        //                     MessageElement::EmoteImages;

        int yExtra = 0;
        if (isCompactEmote) {
            yExtra = (COMPACT_EMOTES_OFFSET / 2) * this->scale;
        }

        element->setPosition(QPoint(element->getRect().x() + xOffset + this->margin.left,
                                    element->getRect().y() + this->lineHeight + yExtra));
    }

    this->lines.push_back({(int)lineStart, QRect(0, this->currentY, this->width, lineHeight)});

    this->lineStart = this->elements.size();
    this->currentX = 0;
    this->currentY += this->lineHeight;
    this->height = this->currentY + (this->margin.bottom * this->scale);
    this->lineHeight = 0;
}

bool MessageLayoutContainer::atStartOfLine()
{
    return this->lineStart == this->elements.size();
}

bool MessageLayoutContainer::fitsInLine(int _width)
{
    return this->currentX + _width <= this->width - this->margin.left - this->margin.right;
}

void MessageLayoutContainer::finish()
{
    if (!this->atStartOfLine()) {
        this->breakLine();
    }

    if (this->lines.size() != 0) {
        this->lines[0].rect.setTop(0);
        this->lines.back().rect.setBottom(this->height);
    }
}

MessageLayoutElement *MessageLayoutContainer::getElementAt(QPoint point)
{
    for (std::unique_ptr<MessageLayoutElement> &element : this->elements) {
        if (element->getRect().contains(point)) {
            return element.get();
        }
    }

    return nullptr;
}

// painting
void MessageLayoutContainer::paintElements(QPainter &painter)
{
    for (const std::unique_ptr<MessageLayoutElement> &element : this->elements) {
        element->paint(painter);
    }
}

void MessageLayoutContainer::paintAnimatedElements(QPainter &painter, int yOffset)
{
    for (const std::unique_ptr<MessageLayoutElement> &element : this->elements) {
        element->paintAnimated(painter, yOffset);
    }
}

void MessageLayoutContainer::paintSelection(QPainter &painter, int messageIndex,
                                            Selection &selection)
{
}

// selection
int MessageLayoutContainer::getSelectionIndex(QPoint point)
{
    if (this->elements.size() == 0) {
        return 0;
    }

    auto line = this->lines.begin();

    for (; line != this->lines.end(); line++) {
        if (line->rect.contains(point)) {
            break;
        }
    }

    assert(line != this->lines.end());

    int lineStart = line->startIndex;
    line++;
    int lineEnd = line == this->lines.end() ? this->elements.size() : line->startIndex;

    int index = 0;

    for (int i = 0; i < lineEnd; i++) {
        // end of line
        if (i == lineEnd) {
            break;
        }

        // before line
        if (i < lineStart) {
            index += this->elements[i]->getSelectionIndexCount();
            continue;
        }

        // this is the word
        if (point.x() > this->elements[i]->getRect().left()) {
            index += this->elements[i]->getMouseOverIndex(point);
            break;
        }
    }

    qDebug() << index;

    return index;
}
}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
