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
    this->lines.clear();

    this->height = 0;
    this->line = 0;
    this->currentX = 0;
    this->currentY = 0;
    this->lineStart = 0;
    this->lineHeight = 0;
    this->charIndex = 0;
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

    if (this->lines.size() != 0) {
        this->lines.back().endIndex = this->lineStart;
        this->lines.back().endCharIndex = this->charIndex;
    }
    this->lines.push_back({(int)lineStart, 0, this->charIndex, 0,
                           QRect(-100000, this->currentY, 200000, lineHeight)});

    for (int i = this->lineStart; i < this->elements.size(); i++) {
        this->charIndex += this->elements[i]->getSelectionIndexCount();
    }

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
        this->lines[0].rect.setTop(-100000);
        this->lines.back().rect.setBottom(100000);
        this->lines.back().endIndex = this->elements.size();
        this->lines.back().endCharIndex = this->charIndex;
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
    singletons::ThemeManager &themeManager = singletons::ThemeManager::getInstance();
    QColor selectionColor = themeManager.messages.selection;

    // don't draw anything
    if (selection.min.messageIndex > messageIndex || selection.max.messageIndex < messageIndex) {
        return;
    }

    // fully selected
    if (selection.min.messageIndex < messageIndex && selection.max.messageIndex > messageIndex) {
        for (Line &line : this->lines) {
            QRect rect = line.rect;

            rect.setTop(std::max(0, rect.top()));
            rect.setBottom(std::min(this->height, rect.bottom()));
            rect.setLeft(this->elements[line.startIndex]->getRect().left());
            rect.setRight(this->elements[line.endIndex - 1]->getRect().right());

            painter.fillRect(rect, selectionColor);
        }
        return;
    }

    int lineIndex = 0;
    int index = 0;

    // start in this message
    if (selection.min.messageIndex == messageIndex) {
        for (; lineIndex < this->lines.size(); lineIndex++) {
            Line &line = this->lines[lineIndex];
            index = line.startCharIndex;

            bool returnAfter = false;
            bool breakAfter = false;
            int x = this->elements[line.startIndex]->getRect().left();
            int r = this->elements[line.endIndex - 1]->getRect().right();

            if (line.endCharIndex < selection.min.charIndex) {
                continue;
            }

            for (int i = line.startIndex; i < line.endIndex; i++) {
                int c = this->elements[i]->getSelectionIndexCount();

                if (index + c > selection.min.charIndex) {
                    x = this->elements[i]->getXFromIndex(selection.min.charIndex - index);

                    // ends in same line
                    if (selection.max.messageIndex == messageIndex &&
                        line.endCharIndex > /*=*/selection.max.charIndex)  //
                    {
                        returnAfter = true;
                        index = line.startCharIndex;
                        for (int i = line.startIndex; i < line.endIndex; i++) {
                            int c = this->elements[i]->getSelectionIndexCount();

                            if (index + c > selection.max.charIndex) {
                                r = this->elements[i]->getXFromIndex(selection.max.charIndex -
                                                                     index);
                                break;
                            }
                            index += c;
                        }
                    }
                    // ends in same line end

                    if (selection.max.messageIndex != messageIndex) {
                        int lineIndex2 = lineIndex + 1;
                        for (; lineIndex2 < this->lines.size(); lineIndex2++) {
                            Line &line = this->lines[lineIndex2];
                            QRect rect = line.rect;

                            rect.setTop(std::max(0, rect.top()));
                            rect.setBottom(std::min(this->height, rect.bottom()));
                            rect.setLeft(this->elements[line.startIndex]->getRect().left());
                            rect.setRight(this->elements[line.endIndex - 1]->getRect().right());

                            painter.fillRect(rect, selectionColor);
                        }
                        returnAfter = true;
                    } else {
                        lineIndex++;
                        breakAfter = true;
                    }

                    break;
                }
                index += c;
            }

            QRect rect = line.rect;

            rect.setTop(std::max(0, rect.top()));
            rect.setBottom(std::min(this->height, rect.bottom()));
            rect.setLeft(x);
            rect.setRight(r);

            painter.fillRect(rect, selectionColor);

            if (returnAfter) {
                return;
            }

            if (breakAfter) {
                break;
            }
        }
    }

    // start in this message
    for (; lineIndex < this->lines.size(); lineIndex++) {
        Line &line = this->lines[lineIndex];
        index = line.startCharIndex;

        // just draw the garbage
        if (line.endCharIndex < /*=*/selection.max.charIndex) {
            QRect rect = line.rect;

            rect.setTop(std::max(0, rect.top()));
            rect.setBottom(std::min(this->height, rect.bottom()));
            rect.setLeft(this->elements[line.startIndex]->getRect().left());
            rect.setRight(this->elements[line.endIndex - 1]->getRect().right());

            painter.fillRect(rect, selectionColor);
            continue;
        }

        int r = this->elements[line.endIndex - 1]->getRect().right();

        for (int i = line.startIndex; i < line.endIndex; i++) {
            int c = this->elements[i]->getSelectionIndexCount();

            if (index + c > selection.max.charIndex) {
                r = this->elements[i]->getXFromIndex(selection.max.charIndex - index);
                break;
            }

            index += c;
        }

        QRect rect = line.rect;

        rect.setTop(std::max(0, rect.top()));
        rect.setBottom(std::min(this->height, rect.bottom()));
        rect.setLeft(this->elements[line.startIndex]->getRect().left());
        rect.setRight(r);

        painter.fillRect(rect, selectionColor);
        break;
    }
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

    int lineStart = line == this->lines.end() ? this->lines.back().startIndex : line->startIndex;
    if (line != this->lines.end()) {
        line++;
    }
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
        if (point.x() < this->elements[i]->getRect().right()) {
            index += this->elements[i]->getMouseOverIndex(point);
            break;
        }

        index += this->elements[i]->getSelectionIndexCount();
    }

    return index;
}

// fourtf: no idea if this is acurate LOL
int MessageLayoutContainer::getLastCharacterIndex() const
{
    if (this->lines.size() == 0) {
        return 0;
    }
    return this->lines.back().endCharIndex;
}

void MessageLayoutContainer::addSelectionText(QString &str, int from, int to)
{
    int index = 0;
    bool xd = true;

    for (std::unique_ptr<MessageLayoutElement> &ele : this->elements) {
        int c = ele->getSelectionIndexCount();

        if (xd) {
            if (index + c > from) {
                ele->addCopyTextToString(str, index - from, to - from);
                xd = false;
            }
        } else {
            if (index + c > from) {
                ele->addCopyTextToString(str, 0, index - to);
                break;
            } else {
                ele->addCopyTextToString(str);
            }
        }

        index += c;
    }
}
}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
