#include "MessageLayoutContainer.hpp"

#include "Application.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "messages/Selection.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/Helpers.hpp"

#include <QDebug>
#include <QPainter>

#define COMPACT_EMOTES_OFFSET 4
#define MAX_UNCOLLAPSED_LINES \
    (getSettings()->collpseMessagesMinLines.getValue())

namespace chatterino {

int MessageLayoutContainer::getHeight() const
{
    return this->height_;
}

int MessageLayoutContainer::getWidth() const
{
    return this->width_;
}

float MessageLayoutContainer::getScale() const
{
    return this->scale_;
}

// methods
void MessageLayoutContainer::begin(int width, float scale, MessageFlags flags)
{
    this->clear();
    this->width_ = width;
    this->scale_ = scale;
    this->flags_ = flags;
    auto mediumFontMetrics =
        getApp()->fonts->getFontMetrics(FontStyle::ChatMedium, scale);
    this->textLineHeight_ = mediumFontMetrics.height();
    this->spaceWidth_ = mediumFontMetrics.horizontalAdvance(' ');
    this->dotdotdotWidth_ = mediumFontMetrics.horizontalAdvance("...");
    this->canAddMessages_ = true;
    this->isCollapsed_ = false;
    this->wasPrevReversed_ = false;
}

void MessageLayoutContainer::clear()
{
    this->elements_.clear();
    this->lines_.clear();

    this->height_ = 0;
    this->line_ = 0;
    this->currentX_ = 0;
    this->currentY_ = 0;
    this->lineStart_ = 0;
    this->lineHeight_ = 0;
    this->charIndex_ = 0;
}

void MessageLayoutContainer::addElement(MessageLayoutElement *element)
{
    if (!this->fitsInLine(element->getRect().width()))
    {
        this->breakLine();
    }

    this->_addElement(element);
}

void MessageLayoutContainer::addElementNoLineBreak(
    MessageLayoutElement *element)
{
    this->_addElement(element);
}

bool MessageLayoutContainer::canAddElements() const
{
    return this->canAddMessages_;
}

void MessageLayoutContainer::_addElement(MessageLayoutElement *element,
                                         bool forceAdd, int prevIndex)
{
    if (!this->canAddElements() && !forceAdd)
    {
        delete element;
        return;
    }

    bool isRTLMode = this->first == FirstWord::RTL && prevIndex != -2;
    bool isAddingMode = prevIndex == -2;

    // This lambda contains the logic for when to step one 'space width' back for compact x emotes
    auto shouldRemoveSpaceBetweenEmotes = [this, prevIndex]() -> bool {
        if (prevIndex == -1 || this->elements_.empty())
        {
            // No previous element found
            return false;
        }

        const auto &lastElement = prevIndex == -2 ? this->elements_.back()
                                                  : this->elements_[prevIndex];

        if (!lastElement)
        {
            return false;
        }

        if (!lastElement->hasTrailingSpace())
        {
            // Last element did not have a trailing space, so we don't need to do anything.
            return false;
        }

        if (lastElement->getLine() != this->line_)
        {
            // Last element was not on the same line as us
            return false;
        }

        // Returns true if the last element was an emote image
        return lastElement->getFlags().has(MessageElementFlag::EmoteImages);
    };

    if (element->getText().isRightToLeft())
    {
        this->containsRTL = true;
    }

    // check the first non-neutral word to see if we should render RTL or LTR
    if (isAddingMode && this->first == FirstWord::Neutral &&
        element->getFlags().has(MessageElementFlag::Text) &&
        !element->getFlags().has(MessageElementFlag::RepliedMessage))
    {
        if (element->getText().isRightToLeft())
        {
            this->first = FirstWord::RTL;
        }
        else if (!isNeutral(element->getText()))
        {
            this->first = FirstWord::LTR;
        }
    }

    // top margin
    if (this->elements_.empty())
    {
        this->currentY_ = int(this->margin.top * this->scale_);
    }

    int elementLineHeight = element->getRect().height();

    // compact emote offset
    bool isCompactEmote =
        !this->flags_.has(MessageFlag::DisableCompactEmotes) &&
        element->getCreator().getFlags().has(MessageElementFlag::EmoteImages);

    if (isCompactEmote)
    {
        elementLineHeight -= COMPACT_EMOTES_OFFSET * this->scale_;
    }

    // update line height
    this->lineHeight_ = std::max(this->lineHeight_, elementLineHeight);

    auto xOffset = 0;
    auto yOffset = 0;

    if (element->getCreator().getFlags().has(
            MessageElementFlag::ChannelPointReward) &&
        element->getCreator().getFlags().hasNone(
            {MessageElementFlag::TwitchEmoteImage}))
    {
        yOffset -= (this->margin.top * this->scale_);
    }

    if (getSettings()->removeSpacesBetweenEmotes &&
        element->getFlags().hasAny({MessageElementFlag::EmoteImages}) &&
        shouldRemoveSpaceBetweenEmotes())
    {
        // Move cursor one 'space width' to the left (right in case of RTL) to combine hug the previous emote
        if (isRTLMode)
        {
            this->currentX_ += this->spaceWidth_;
        }
        else
        {
            this->currentX_ -= this->spaceWidth_;
        }
    }

    if (isRTLMode)
    {
        // shift by width since we are calculating according to top right in RTL mode
        // but setPosition wants top left
        xOffset -= element->getRect().width();
    }

    // set move element
    element->setPosition(
        QPoint(this->currentX_ + xOffset,
               this->currentY_ - element->getRect().height() + yOffset));

    element->setLine(this->line_);

    // add element
    if (isAddingMode)
    {
        this->elements_.push_back(
            std::unique_ptr<MessageLayoutElement>(element));
    }

    // set current x
    if (isRTLMode)
    {
        this->currentX_ -= element->getRect().width();
    }
    else
    {
        this->currentX_ += element->getRect().width();
    }

    if (element->hasTrailingSpace())
    {
        if (isRTLMode)
        {
            this->currentX_ -= this->spaceWidth_;
        }
        else
        {
            this->currentX_ += this->spaceWidth_;
        }
    }
}

void MessageLayoutContainer::reorderRTL(int firstTextIndex)
{
    if (this->elements_.empty())
    {
        return;
    }

    int startIndex = static_cast<int>(this->lineStart_);
    int endIndex = static_cast<int>(this->elements_.size()) - 1;

    if (firstTextIndex >= endIndex || startIndex >= this->elements_.size())
    {
        return;
    }
    startIndex = std::max(startIndex, firstTextIndex);

    std::vector<int> correctSequence;
    std::stack<int> swappedSequence;

    // we reverse a sequence of words if it's opposite to the text direction
    // the second condition below covers the possible three cases:
    // 1 - if we are in RTL mode (first non-neutral word is RTL)
    // we render RTL, reversing LTR sequences,
    // 2 - if we are in LTR mode (first non-neutral word is LTR or all words are neutral)
    // we render LTR, reversing RTL sequences
    // 3 - neutral words follow previous words, we reverse a neutral word when the previous word was reversed

    // the first condition checks if a neutral word is treated as a RTL word
    // this is used later to add U+202B (RTL embedding) character signal to
    // fix punctuation marks and mixing embedding LTR in an RTL word
    // this can happen in two cases:
    // 1 - in RTL mode, the previous word should be RTL (i.e. not reversed)
    // 2 - in LTR mode, the previous word should be RTL (i.e. reversed)
    for (int i = startIndex; i <= endIndex; i++)
    {
        auto &element = this->elements_[i];

        const auto neutral = isNeutral(element->getText());
        const auto neutralOrUsername =
            neutral ||
            element->getFlags().hasAny({MessageElementFlag::BoldUsername,
                                        MessageElementFlag::NonBoldUsername});

        if (neutral &&
            ((this->first == FirstWord::RTL && !this->wasPrevReversed_) ||
             (this->first == FirstWord::LTR && this->wasPrevReversed_)))
        {
            element->reversedNeutral = true;
        }
        if (((element->getText().isRightToLeft() !=
              (this->first == FirstWord::RTL)) &&
             !neutralOrUsername) ||
            (neutralOrUsername && this->wasPrevReversed_))
        {
            swappedSequence.push(i);
            this->wasPrevReversed_ = true;
        }
        else
        {
            while (!swappedSequence.empty())
            {
                correctSequence.push_back(swappedSequence.top());
                swappedSequence.pop();
            }
            correctSequence.push_back(i);
            this->wasPrevReversed_ = false;
        }
    }
    while (!swappedSequence.empty())
    {
        correctSequence.push_back(swappedSequence.top());
        swappedSequence.pop();
    }

    // render right to left if we are in RTL mode, otherwise LTR
    if (this->first == FirstWord::RTL)
    {
        this->currentX_ = this->elements_[endIndex]->getRect().right();
    }
    else
    {
        this->currentX_ = this->elements_[startIndex]->getRect().left();
    }
    // manually do the first call with -1 as previous index
    if (this->canAddElements())
    {
        this->_addElement(this->elements_[correctSequence[0]].get(), false, -1);
    }

    for (int i = 1; i < correctSequence.size() && this->canAddElements(); i++)
    {
        this->_addElement(this->elements_[correctSequence[i]].get(), false,
                          correctSequence[i - 1]);
    }
}

void MessageLayoutContainer::breakLine()
{
    if (this->containsRTL)
    {
        for (int i = 0; i < this->elements_.size(); i++)
        {
            if (this->elements_[i]->getFlags().has(
                    MessageElementFlag::Username))
            {
                this->reorderRTL(i + 1);
                break;
            }
        }
    }

    int xOffset = 0;

    if (this->flags_.has(MessageFlag::Centered) && this->elements_.size() > 0)
    {
        const int marginOffset = int(this->margin.left * this->scale_) +
                                 int(this->margin.right * this->scale_);
        xOffset = (width_ - marginOffset -
                   this->elements_.at(this->elements_.size() - 1)
                       ->getRect()
                       .right()) /
                  2;
    }

    for (size_t i = lineStart_; i < this->elements_.size(); i++)
    {
        MessageLayoutElement *element = this->elements_.at(i).get();

        bool isCompactEmote =
            !this->flags_.has(MessageFlag::DisableCompactEmotes) &&
            element->getCreator().getFlags().has(
                MessageElementFlag::EmoteImages);

        int yExtra = 0;
        if (isCompactEmote)
        {
            yExtra = (COMPACT_EMOTES_OFFSET / 2) * this->scale_;
        }

        element->setPosition(
            QPoint(element->getRect().x() + xOffset +
                       int(this->margin.left * this->scale_),
                   element->getRect().y() + this->lineHeight_ + yExtra));
    }

    if (!this->lines_.empty())
    {
        this->lines_.back().endIndex = this->lineStart_;
        this->lines_.back().endCharIndex = this->charIndex_;
    }
    this->lines_.push_back(
        {(int)lineStart_, 0, this->charIndex_, 0,
         QRect(-100000, this->currentY_, 200000, lineHeight_)});

    for (auto i = this->lineStart_; i < this->elements_.size(); i++)
    {
        this->charIndex_ += this->elements_[i]->getSelectionIndexCount();
    }

    this->lineStart_ = this->elements_.size();
    //    this->currentX = (int)(this->scale * 8);

    if (this->canCollapse() && line_ + 1 >= MAX_UNCOLLAPSED_LINES)
    {
        this->canAddMessages_ = false;
        return;
    }

    this->currentX_ = 0;
    this->currentY_ += this->lineHeight_;
    this->height_ = this->currentY_ + int(this->margin.bottom * this->scale_);
    this->lineHeight_ = 0;
    this->line_++;
}

bool MessageLayoutContainer::atStartOfLine()
{
    return this->lineStart_ == this->elements_.size();
}

bool MessageLayoutContainer::fitsInLine(int _width)
{
    return this->currentX_ + _width <=
           (this->width_ - int(this->margin.left * this->scale_) -
            int(this->margin.right * this->scale_) -
            (this->line_ + 1 == MAX_UNCOLLAPSED_LINES ? this->dotdotdotWidth_
                                                      : 0));
}

void MessageLayoutContainer::end()
{
    if (!this->canAddElements())
    {
        static TextElement dotdotdot("...", MessageElementFlag::Collapsed,
                                     MessageColor::Link);
        static QString dotdotdotText("...");

        auto *element = new TextLayoutElement(
            dotdotdot, dotdotdotText,
            QSize(this->dotdotdotWidth_, this->textLineHeight_),
            QColor("#00D80A"), FontStyle::ChatMediumBold, this->scale_);

        if (this->first == FirstWord::RTL)
        {
            // Shift all elements in the next line to the left
            for (int i = this->lines_.back().startIndex;
                 i < this->elements_.size(); i++)
            {
                QPoint prevPos = this->elements_[i]->getRect().topLeft();
                this->elements_[i]->setPosition(
                    QPoint(prevPos.x() + this->dotdotdotWidth_, prevPos.y()));
            }
        }
        this->_addElement(element, true);
        this->isCollapsed_ = true;
    }

    if (!this->atStartOfLine())
    {
        this->breakLine();
    }

    this->height_ += this->lineHeight_;

    if (!this->lines_.empty())
    {
        this->lines_[0].rect.setTop(-100000);
        this->lines_.back().rect.setBottom(100000);
        this->lines_.back().endIndex = this->elements_.size();
        this->lines_.back().endCharIndex = this->charIndex_;
    }
}

bool MessageLayoutContainer::canCollapse()
{
    return getSettings()->collpseMessagesMinLines.getValue() > 0 &&
           this->flags_.has(MessageFlag::Collapsed);
}

bool MessageLayoutContainer::isCollapsed() const
{
    return this->isCollapsed_;
}

MessageLayoutElement *MessageLayoutContainer::getElementAt(QPoint point)
{
    for (std::unique_ptr<MessageLayoutElement> &element : this->elements_)
    {
        if (element->getRect().contains(point))
        {
            return element.get();
        }
    }

    return nullptr;
}

// painting
void MessageLayoutContainer::paintElements(QPainter &painter,
                                           const MessagePaintContext &ctx)
{
    for (const std::unique_ptr<MessageLayoutElement> &element : this->elements_)
    {
#ifdef FOURTF
        painter.setPen(QColor(0, 255, 0));
        painter.drawRect(element->getRect());
#endif

        element->paint(painter, ctx.messageColors);
    }
}

void MessageLayoutContainer::paintAnimatedElements(QPainter &painter,
                                                   int yOffset)
{
    for (const std::unique_ptr<MessageLayoutElement> &element : this->elements_)
    {
        element->paintAnimated(painter, yOffset);
    }
}

void MessageLayoutContainer::paintSelection(QPainter &painter,
                                            size_t messageIndex,
                                            const Selection &selection,
                                            int yOffset)
{
    auto *app = getApp();
    QColor selectionColor = app->themes->messages.selection;

    // don't draw anything
    if (selection.selectionMin.messageIndex > messageIndex ||
        selection.selectionMax.messageIndex < messageIndex)
    {
        return;
    }

    // fully selected
    if (selection.selectionMin.messageIndex < messageIndex &&
        selection.selectionMax.messageIndex > messageIndex)
    {
        for (Line &line : this->lines_)
        {
            QRect rect = line.rect;

            rect.setTop(std::max(0, rect.top()) + yOffset);
            rect.setBottom(std::min(this->height_, rect.bottom()) + yOffset);
            rect.setLeft(this->elements_[line.startIndex]->getRect().left());
            rect.setRight(
                this->elements_[line.endIndex - 1]->getRect().right());

            painter.fillRect(rect, selectionColor);
        }
        return;
    }

    int lineIndex = 0;
    int index = 0;

    // start in this message
    if (selection.selectionMin.messageIndex == messageIndex)
    {
        for (; lineIndex < this->lines_.size(); lineIndex++)
        {
            Line &line = this->lines_[lineIndex];
            index = line.startCharIndex;

            bool returnAfter = false;
            bool breakAfter = false;
            int x = this->elements_[line.startIndex]->getRect().left();
            int r = this->elements_[line.endIndex - 1]->getRect().right();

            if (line.endCharIndex <= selection.selectionMin.charIndex)
            {
                continue;
            }

            for (int i = line.startIndex; i < line.endIndex; i++)
            {
                int c = this->elements_[i]->getSelectionIndexCount();

                if (index + c > selection.selectionMin.charIndex)
                {
                    x = this->elements_[i]->getXFromIndex(
                        selection.selectionMin.charIndex - index);

                    // ends in same line
                    if (selection.selectionMax.messageIndex == messageIndex &&
                        line.endCharIndex >
                            /*=*/selection.selectionMax.charIndex)
                    {
                        returnAfter = true;
                        index = line.startCharIndex;
                        for (int i = line.startIndex; i < line.endIndex; i++)
                        {
                            int c =
                                this->elements_[i]->getSelectionIndexCount();

                            if (index + c > selection.selectionMax.charIndex)
                            {
                                r = this->elements_[i]->getXFromIndex(
                                    selection.selectionMax.charIndex - index);
                                break;
                            }
                            index += c;
                        }
                    }
                    // ends in same line end

                    if (selection.selectionMax.messageIndex != messageIndex)
                    {
                        int lineIndex2 = lineIndex + 1;
                        for (; lineIndex2 < this->lines_.size(); lineIndex2++)
                        {
                            Line &line2 = this->lines_[lineIndex2];
                            QRect rect = line2.rect;

                            rect.setTop(std::max(0, rect.top()) + yOffset);
                            rect.setBottom(
                                std::min(this->height_, rect.bottom()) +
                                yOffset);
                            rect.setLeft(this->elements_[line2.startIndex]
                                             ->getRect()
                                             .left());
                            rect.setRight(this->elements_[line2.endIndex - 1]
                                              ->getRect()
                                              .right());

                            painter.fillRect(rect, selectionColor);
                        }
                        returnAfter = true;
                    }
                    else
                    {
                        lineIndex++;
                        breakAfter = true;
                    }

                    break;
                }
                index += c;
            }

            QRect rect = line.rect;

            rect.setTop(std::max(0, rect.top()) + yOffset);
            rect.setBottom(std::min(this->height_, rect.bottom()) + yOffset);
            rect.setLeft(x);
            rect.setRight(r);

            painter.fillRect(rect, selectionColor);

            if (returnAfter)
            {
                return;
            }

            if (breakAfter)
            {
                break;
            }
        }
    }

    // start in this message
    for (; lineIndex < this->lines_.size(); lineIndex++)
    {
        Line &line = this->lines_[lineIndex];
        index = line.startCharIndex;

        // just draw the garbage
        if (line.endCharIndex < /*=*/selection.selectionMax.charIndex)
        {
            QRect rect = line.rect;

            rect.setTop(std::max(0, rect.top()) + yOffset);
            rect.setBottom(std::min(this->height_, rect.bottom()) + yOffset);
            rect.setLeft(this->elements_[line.startIndex]->getRect().left());
            rect.setRight(
                this->elements_[line.endIndex - 1]->getRect().right());

            painter.fillRect(rect, selectionColor);
            continue;
        }

        int r = this->elements_[line.endIndex - 1]->getRect().right();

        for (int i = line.startIndex; i < line.endIndex; i++)
        {
            int c = this->elements_[i]->getSelectionIndexCount();

            if (index + c > selection.selectionMax.charIndex)
            {
                r = this->elements_[i]->getXFromIndex(
                    selection.selectionMax.charIndex - index);
                break;
            }

            index += c;
        }

        QRect rect = line.rect;

        rect.setTop(std::max(0, rect.top()) + yOffset);
        rect.setBottom(std::min(this->height_, rect.bottom()) + yOffset);
        rect.setLeft(this->elements_[line.startIndex]->getRect().left());
        rect.setRight(r);

        painter.fillRect(rect, selectionColor);
        break;
    }
}

// selection
int MessageLayoutContainer::getSelectionIndex(QPoint point)
{
    if (this->elements_.empty())
    {
        return 0;
    }

    auto line = this->lines_.begin();

    for (; line != this->lines_.end(); line++)
    {
        if (line->rect.contains(point))
        {
            break;
        }
    }

    int lineStart = line == this->lines_.end() ? this->lines_.back().startIndex
                                               : line->startIndex;
    if (line != this->lines_.end())
    {
        line++;
    }
    int lineEnd =
        line == this->lines_.end() ? this->elements_.size() : line->startIndex;

    int index = 0;

    for (int i = 0; i < lineEnd; i++)
    {
        auto &&element = this->elements_[i];

        // end of line
        if (i == lineEnd)
        {
            break;
        }

        // before line
        if (i < lineStart)
        {
            index += element->getSelectionIndexCount();
            continue;
        }

        // this is the word
        auto rightMargin = element->hasTrailingSpace() ? this->spaceWidth_ : 0;

        if (point.x() <= element->getRect().right() + rightMargin)
        {
            index += element->getMouseOverIndex(point);
            break;
        }

        index += element->getSelectionIndexCount();
    }

    return index;
}

// fourtf: no idea if this is acurate LOL
int MessageLayoutContainer::getLastCharacterIndex() const
{
    if (this->lines_.empty())
    {
        return 0;
    }
    return this->lines_.back().endCharIndex;
}

int MessageLayoutContainer::getFirstMessageCharacterIndex() const
{
    static FlagsEnum<MessageElementFlag> skippedFlags;
    skippedFlags.set(MessageElementFlag::RepliedMessage);
    skippedFlags.set(MessageElementFlag::Timestamp);
    skippedFlags.set(MessageElementFlag::Badges);
    skippedFlags.set(MessageElementFlag::Username);

    // Get the index of the first character of the real message
    int index = 0;
    for (const auto &element : this->elements_)
    {
        if (element->getFlags().hasAny(skippedFlags))
        {
            index += element->getSelectionIndexCount();
        }
        else
        {
            break;
        }
    }
    return index;
}

void MessageLayoutContainer::addSelectionText(QString &str, uint32_t from,
                                              uint32_t to, CopyMode copymode)
{
    uint32_t index = 0;
    bool first = true;

    for (auto &element : this->elements_)
    {
        if (copymode != CopyMode::Everything &&
            element->getCreator().getFlags().has(
                MessageElementFlag::RepliedMessage))
        {
            // Don't include the message being replied to
            continue;
        }

        if (copymode == CopyMode::OnlyTextAndEmotes)
        {
            if (element->getCreator().getFlags().hasAny(
                    {MessageElementFlag::Timestamp,
                     MessageElementFlag::Username, MessageElementFlag::Badges}))
            {
                continue;
            }
        }

        auto indexCount = element->getSelectionIndexCount();

        if (first)
        {
            if (index + indexCount > from)
            {
                element->addCopyTextToString(str, from - index, to - index);
                first = false;

                if (index + indexCount > to)
                {
                    break;
                }
            }
        }
        else
        {
            if (index + indexCount > to)
            {
                element->addCopyTextToString(str, 0, to - index);
                break;
            }

            element->addCopyTextToString(str);
        }

        index += indexCount;
    }
}

}  // namespace chatterino
