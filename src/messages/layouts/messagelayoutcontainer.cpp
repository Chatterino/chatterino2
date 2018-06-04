#include "messagelayoutcontainer.hpp"

#include "application.hpp"
#include "messagelayoutelement.hpp"
#include "messages/selection.hpp"
#include "singletons/settingsmanager.hpp"

#include <QDebug>
#include <QPainter>

#define COMPACT_EMOTES_OFFSET 6
#define MAX_UNCOLLAPSED_LINES (getApp()->settings->collpseMessagesMinLines.getValue())

namespace chatterino {
namespace messages {
namespace layouts {

int MessageLayoutContainer::getHeight() const
{
    return this->height;
}

int MessageLayoutContainer::getWidth() const
{
    return this->width;
}

float MessageLayoutContainer::getScale() const
{
    return this->scale;
}

// methods
void MessageLayoutContainer::begin(int _width, float _scale, Message::MessageFlags _flags)
{
    this->clear();
    this->width = _width;
    this->scale = _scale;
    this->flags = _flags;
    auto mediumFontMetrics = getApp()->fonts->getFontMetrics(FontStyle::ChatMedium, _scale);
    this->textLineHeight = mediumFontMetrics.height();
    this->spaceWidth = mediumFontMetrics.width(' ');
    this->dotdotdotWidth = mediumFontMetrics.width("...");
    this->_canAddMessages = true;
    this->_isCollapsed = false;
}

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

bool MessageLayoutContainer::canAddElements()
{
    return this->_canAddMessages;
}

void MessageLayoutContainer::_addElement(MessageLayoutElement *element, bool forceAdd)
{
    if (!this->canAddElements() && !forceAdd) {
        delete element;
        return;
    }

    // top margin
    if (this->elements.size() == 0) {
        this->currentY = this->margin.top * this->scale;
    }

    int newLineHeight = element->getRect().height();

    // compact emote offset
    bool isCompactEmote = !(this->flags & Message::DisableCompactEmotes) &&
                          element->getCreator().getFlags() & MessageElement::EmoteImages;

    if (isCompactEmote) {
        newLineHeight -= COMPACT_EMOTES_OFFSET * this->scale;
    }

    // update line height
    this->lineHeight = std::max(this->lineHeight, newLineHeight);

    // set move element
    element->setPosition(QPoint(this->currentX, this->currentY - element->getRect().height()));

    // add element
    this->elements.push_back(std::unique_ptr<MessageLayoutElement>(element));

    // set current x
    this->currentX += element->getRect().width();

    if (element->hasTrailingSpace()) {
        this->currentX += this->spaceWidth;
    }
}

void MessageLayoutContainer::breakLine()
{
    int xOffset = 0;

    if (this->flags & Message::Centered && this->elements.size() > 0) {
        xOffset = (width - this->elements.at(this->elements.size() - 1)->getRect().right()) / 2;
    }

    for (size_t i = lineStart; i < this->elements.size(); i++) {
        MessageLayoutElement *element = this->elements.at(i).get();

        bool isCompactEmote = !(this->flags & Message::DisableCompactEmotes) &&
                              element->getCreator().getFlags() & MessageElement::EmoteImages;

        int yExtra = 0;
        if (isCompactEmote) {
            yExtra = (COMPACT_EMOTES_OFFSET / 2) * this->scale;
        }

        //        if (element->getCreator().getFlags() & MessageElement::Badges) {
        if (element->getRect().height() < this->textLineHeight) {
            yExtra -= (this->textLineHeight - element->getRect().height()) / 2;
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
    //    this->currentX = (int)(this->scale * 8);

    if (this->canCollapse() && line + 1 >= MAX_UNCOLLAPSED_LINES) {
        this->_canAddMessages = false;
        return;
    }

    this->currentX = 0;
    this->currentY += this->lineHeight;
    this->height = this->currentY + (this->margin.bottom * this->scale);
    this->lineHeight = 0;
    this->line++;
}

bool MessageLayoutContainer::atStartOfLine()
{
    return this->lineStart == this->elements.size();
}

bool MessageLayoutContainer::fitsInLine(int _width)
{
    return this->currentX + _width <=
           (this->width - this->margin.left - this->margin.right -
            (this->line + 1 == MAX_UNCOLLAPSED_LINES ? this->dotdotdotWidth : 0));
}

void MessageLayoutContainer::end()
{
    if (!this->canAddElements()) {
        static TextElement dotdotdot("...", MessageElement::Collapsed, MessageColor::Link);
        static QString dotdotdotText("...");

        auto *element = new TextLayoutElement(
            dotdotdot, dotdotdotText, QSize(this->dotdotdotWidth, this->textLineHeight),
            QColor("#00D80A"), FontStyle::ChatMediumBold, this->scale);

        // getApp()->themes->messages.textColors.system
        this->_addElement(element, true);
        this->_isCollapsed = true;
    }

    if (!this->atStartOfLine()) {
        this->breakLine();
    }

    this->height += this->lineHeight;

    if (this->lines.size() != 0) {
        this->lines[0].rect.setTop(-100000);
        this->lines.back().rect.setBottom(100000);
        this->lines.back().endIndex = this->elements.size();
        this->lines.back().endCharIndex = this->charIndex;
    }
}

bool MessageLayoutContainer::canCollapse()
{
    return getApp()->settings->collpseMessagesMinLines.getValue() > 0 &&
           this->flags & Message::MessageFlags::Collapsed;
}

bool MessageLayoutContainer::isCollapsed()
{
    return this->_isCollapsed;
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
#ifdef FOURTF
        painter.setPen(QColor(0, 255, 0));
        painter.drawRect(element->getRect());
#endif

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
                                            Selection &selection, int yOffset)
{
    auto app = getApp();
    QColor selectionColor = app->themes->messages.selection;

    // don't draw anything
    if (selection.selectionMin.messageIndex > messageIndex ||
        selection.selectionMax.messageIndex < messageIndex) {
        return;
    }

    // fully selected
    if (selection.selectionMin.messageIndex < messageIndex &&
        selection.selectionMax.messageIndex > messageIndex) {
        for (Line &line : this->lines) {
            QRect rect = line.rect;

            rect.setTop(std::max(0, rect.top()) + yOffset);
            rect.setBottom(std::min(this->height, rect.bottom()) + yOffset);
            rect.setLeft(this->elements[line.startIndex]->getRect().left());
            rect.setRight(this->elements[line.endIndex - 1]->getRect().right());

            painter.fillRect(rect, selectionColor);
        }
        return;
    }

    int lineIndex = 0;
    int index = 0;

    // start in this message
    if (selection.selectionMin.messageIndex == messageIndex) {
        for (; lineIndex < this->lines.size(); lineIndex++) {
            Line &line = this->lines[lineIndex];
            index = line.startCharIndex;

            bool returnAfter = false;
            bool breakAfter = false;
            int x = this->elements[line.startIndex]->getRect().left();
            int r = this->elements[line.endIndex - 1]->getRect().right();

            if (line.endCharIndex < selection.selectionMin.charIndex) {
                continue;
            }

            for (int i = line.startIndex; i < line.endIndex; i++) {
                int c = this->elements[i]->getSelectionIndexCount();

                if (index + c > selection.selectionMin.charIndex) {
                    x = this->elements[i]->getXFromIndex(selection.selectionMin.charIndex - index);

                    // ends in same line
                    if (selection.selectionMax.messageIndex == messageIndex &&
                        line.endCharIndex > /*=*/selection.selectionMax.charIndex)  //
                    {
                        returnAfter = true;
                        index = line.startCharIndex;
                        for (int i = line.startIndex; i < line.endIndex; i++) {
                            int c = this->elements[i]->getSelectionIndexCount();

                            if (index + c > selection.selectionMax.charIndex) {
                                r = this->elements[i]->getXFromIndex(
                                    selection.selectionMax.charIndex - index);
                                break;
                            }
                            index += c;
                        }
                    }
                    // ends in same line end

                    if (selection.selectionMax.messageIndex != messageIndex) {
                        int lineIndex2 = lineIndex + 1;
                        for (; lineIndex2 < this->lines.size(); lineIndex2++) {
                            Line &line = this->lines[lineIndex2];
                            QRect rect = line.rect;

                            rect.setTop(std::max(0, rect.top()) + yOffset);
                            rect.setBottom(std::min(this->height, rect.bottom()) + yOffset);
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

            rect.setTop(std::max(0, rect.top()) + yOffset);
            rect.setBottom(std::min(this->height, rect.bottom()) + yOffset);
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
        if (line.endCharIndex < /*=*/selection.selectionMax.charIndex) {
            QRect rect = line.rect;

            rect.setTop(std::max(0, rect.top()) + yOffset);
            rect.setBottom(std::min(this->height, rect.bottom()) + yOffset);
            rect.setLeft(this->elements[line.startIndex]->getRect().left());
            rect.setRight(this->elements[line.endIndex - 1]->getRect().right());

            painter.fillRect(rect, selectionColor);
            continue;
        }

        int r = this->elements[line.endIndex - 1]->getRect().right();

        for (int i = line.startIndex; i < line.endIndex; i++) {
            int c = this->elements[i]->getSelectionIndexCount();

            if (index + c > selection.selectionMax.charIndex) {
                r = this->elements[i]->getXFromIndex(selection.selectionMax.charIndex - index);
                break;
            }

            index += c;
        }

        QRect rect = line.rect;

        rect.setTop(std::max(0, rect.top()) + yOffset);
        rect.setBottom(std::min(this->height, rect.bottom()) + yOffset);
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
    bool first = true;

    for (std::unique_ptr<MessageLayoutElement> &ele : this->elements) {
        int c = ele->getSelectionIndexCount();

        if (first) {
            if (index + c > from) {
                ele->addCopyTextToString(str, from - index, to - index);
                first = false;

                if (index + c > to) {
                    break;
                }
            }
        } else {
            if (index + c > to) {
                ele->addCopyTextToString(str, 0, to - index);
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
