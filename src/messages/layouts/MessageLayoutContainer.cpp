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
#include <QMargins>
#include <QPainter>

#include <optional>

#define COMPACT_EMOTES_OFFSET 4
#define MAX_UNCOLLAPSED_LINES \
    (getSettings()->collpseMessagesMinLines.getValue())

namespace {

constexpr const QMargins MARGIN{8, 4, 8, 4};

}  // namespace

namespace chatterino {

void MessageLayoutContainer::beginLayout(int width, float scale,
                                         MessageFlags flags)
{
    this->elements_.clear();
    this->lines_.clear();

    this->line_ = 0;
    this->currentX_ = 0;
    this->currentY_ = 0;
    this->lineStart_ = 0;
    this->lineHeight_ = 0;
    this->charIndex_ = 0;

    this->width_ = width;
    this->height_ = 0;
    this->scale_ = scale;
    this->flags_ = flags;
    auto mediumFontMetrics =
        getIApp()->getFonts()->getFontMetrics(FontStyle::ChatMedium, scale);
    this->textLineHeight_ = mediumFontMetrics.height();
    this->spaceWidth_ = mediumFontMetrics.horizontalAdvance(' ');
    this->dotdotdotWidth_ = mediumFontMetrics.horizontalAdvance("...");
    this->currentWordId_ = 0;
    this->canAddMessages_ = true;
    this->isCollapsed_ = false;
    this->wasPrevReversed_ = false;
}

void MessageLayoutContainer::endLayout()
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
            for (auto i = this->lines_.back().startIndex;
                 i < this->elements_.size(); i++)
            {
                QPoint prevPos = this->elements_[i]->getRect().topLeft();
                this->elements_[i]->setPosition(
                    QPoint(prevPos.x() + this->dotdotdotWidth_, prevPos.y()));
            }
        }
        this->addElement(element, true, -2);
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

    if (!this->elements_.empty())
    {
        this->elements_.back()->setTrailingSpace(false);
    }
}

void MessageLayoutContainer::addElement(MessageLayoutElement *element)
{
    if (!this->fitsInLine(element->getRect().width()))
    {
        this->breakLine();
    }

    this->addElement(element, false, -2);
}

void MessageLayoutContainer::addElementNoLineBreak(
    MessageLayoutElement *element)
{
    this->addElement(element, false, -2);
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
        const int marginOffset = int(MARGIN.left() * this->scale_) +
                                 int(MARGIN.right() * this->scale_);
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
                       int(MARGIN.left() * this->scale_),
                   element->getRect().y() + this->lineHeight_ + yExtra));
    }

    if (!this->lines_.empty())
    {
        this->lines_.back().endIndex = this->lineStart_;
        this->lines_.back().endCharIndex = this->charIndex_;
    }
    this->lines_.push_back({
        .startIndex = lineStart_,
        .endIndex = 0,
        .startCharIndex = this->charIndex_,
        .endCharIndex = 0,
        .rect = QRect(-100000, this->currentY_, 200000, lineHeight_),
    });

    for (auto i = this->lineStart_; i < this->elements_.size(); i++)
    {
        this->charIndex_ += this->elements_[i]->getSelectionIndexCount();
    }

    this->lineStart_ = this->elements_.size();
    //    this->currentX = (int)(this->scale * 8);

    if (this->canCollapse() && this->line_ + 1 >= MAX_UNCOLLAPSED_LINES)
    {
        this->canAddMessages_ = false;
        return;
    }

    this->currentX_ = 0;
    this->currentY_ += this->lineHeight_;
    this->height_ = this->currentY_ + int(MARGIN.bottom() * this->scale_);
    this->lineHeight_ = 0;
    this->line_++;
}

void MessageLayoutContainer::paintElements(QPainter &painter,
                                           const MessagePaintContext &ctx) const
{
#ifdef FOURTF
    static constexpr std::array<QColor, 5> lineColors{
        QColor{255, 0, 0, 60},    // RED
        QColor{0, 255, 0, 60},    // GREEN
        QColor{0, 0, 255, 60},    // BLUE
        QColor{255, 0, 255, 60},  // PINk
        QColor{0, 255, 255, 60},  // CYAN
    };

    int lineNum = 0;
    for (const auto &line : this->lines_)
    {
        const auto &color = lineColors[lineNum++ % 5];
        painter.fillRect(line.rect, color);
    }
#endif

    for (const auto &element : this->elements_)
    {
#ifdef FOURTF
        painter.setPen(QColor(0, 255, 0));
        painter.drawRect(element->getRect());
#endif

        element->paint(painter, ctx.messageColors);
    }
}

bool MessageLayoutContainer::paintAnimatedElements(QPainter &painter,
                                                   int yOffset) const
{
    bool anyAnimatedElement = false;
    for (const auto &element : this->elements_)
    {
        anyAnimatedElement |= element->paintAnimated(painter, yOffset);
    }
    return anyAnimatedElement;
}

void MessageLayoutContainer::paintSelection(QPainter &painter,
                                            const size_t messageIndex,
                                            const Selection &selection,
                                            const int yOffset) const
{
    if (selection.selectionMin.messageIndex > messageIndex ||
        selection.selectionMax.messageIndex < messageIndex)
    {
        // This message is not part of the selection, don't draw anything
        return;
    }

    const auto selectionColor = getTheme()->messages.selection;

    if (selection.selectionMin.messageIndex < messageIndex &&
        selection.selectionMax.messageIndex > messageIndex)
    {
        // The selection fully covers this message
        // Paint all lines completely

        for (const Line &line : this->lines_)
        {
            // Fully paint a selection rectangle over all lines
            auto left = this->elements_[line.startIndex]->getRect().left();
            auto right = this->elements_[line.endIndex - 1]->getRect().right();
            this->paintSelectionRect(painter, line, left, right, yOffset,
                                     selectionColor);
        }

        return;
    }

    size_t lineIndex = 0;

    if (selection.selectionMin.messageIndex == messageIndex)
    {
        auto oLineIndex = this->paintSelectionStart(painter, messageIndex,
                                                    selection, yOffset);

        if (!oLineIndex)
        {
            // There's no more selection to be drawn in this message
            return;
        }

        // There's further selection to be painted in this message
        lineIndex = *oLineIndex;
    }

    // Paint the selection starting at lineIndex
    this->paintSelectionEnd(painter, lineIndex, selection, yOffset);
}

void MessageLayoutContainer::addSelectionText(QString &str, uint32_t from,
                                              uint32_t to,
                                              CopyMode copymode) const
{
    uint32_t index = 0;
    bool first = true;

    for (const auto &element : this->elements_)
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
            if (element->getCreator().getFlags().hasAny({
                    MessageElementFlag::Timestamp,
                    MessageElementFlag::Username,
                    MessageElementFlag::Badges,
                    MessageElementFlag::ChannelName,
                }))
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

                if (index + indexCount >= to)
                {
                    break;
                }
            }
        }
        else
        {
            if (index + indexCount >= to)
            {
                element->addCopyTextToString(str, 0, to - index);
                break;
            }

            element->addCopyTextToString(str);
        }

        index += indexCount;
    }
}

MessageLayoutElement *MessageLayoutContainer::getElementAt(QPoint point) const
{
    for (const auto &element : this->elements_)
    {
        if (element->getRect().contains(point))
        {
            return element.get();
        }
    }

    return nullptr;
}

size_t MessageLayoutContainer::getSelectionIndex(QPoint point) const
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

    auto lineStart = line == this->lines_.end() ? this->lines_.back().startIndex
                                                : line->startIndex;
    if (line != this->lines_.end())
    {
        line++;
    }
    auto lineEnd =
        line == this->lines_.end() ? this->elements_.size() : line->startIndex;

    size_t index = 0;

    for (auto i = 0; i < lineEnd; i++)
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

size_t MessageLayoutContainer::getFirstMessageCharacterIndex() const
{
    static const FlagsEnum<MessageElementFlag> skippedFlags{
        MessageElementFlag::RepliedMessage, MessageElementFlag::Timestamp,
        MessageElementFlag::ModeratorTools, MessageElementFlag::Badges,
        MessageElementFlag::Username,
    };

    // Get the index of the first character of the real message
    size_t index = 0;
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

std::pair<int, int> MessageLayoutContainer::getWordBounds(
    const MessageLayoutElement *hoveredElement) const
{
    if (this->elements_.empty())
    {
        return {0, 0};
    }

    size_t index = 0;
    size_t wordStart = 0;

    for (; index < this->elements_.size(); index++)
    {
        const auto &element = this->elements_[index];
        if (element->getWordId() == hoveredElement->getWordId())
        {
            break;
        }

        wordStart += element->getSelectionIndexCount();
    }

    size_t wordEnd = wordStart;

    for (; index < this->elements_.size(); index++)
    {
        const auto &element = this->elements_[index];
        if (element->getWordId() != hoveredElement->getWordId())
        {
            break;
        }

        wordEnd += element->getSelectionIndexCount();
    }

    const auto *lastElementInSelection = this->elements_[index - 1].get();
    if (lastElementInSelection->hasTrailingSpace())
    {
        wordEnd--;
    }

    return {wordStart, wordEnd};
}

size_t MessageLayoutContainer::getLastCharacterIndex() const
{
    if (this->lines_.empty())
    {
        return 0;
    }

    return this->lines_.back().endCharIndex;
}

int MessageLayoutContainer::getWidth() const
{
    return this->width_;
}

int MessageLayoutContainer::getHeight() const
{
    return this->height_;
}

float MessageLayoutContainer::getScale() const
{
    return this->scale_;
}

bool MessageLayoutContainer::isCollapsed() const
{
    return this->isCollapsed_;
}

bool MessageLayoutContainer::atStartOfLine() const
{
    return this->lineStart_ == this->elements_.size();
}

bool MessageLayoutContainer::fitsInLine(int width) const
{
    return width <= this->remainingWidth();
}

int MessageLayoutContainer::remainingWidth() const
{
    return (this->width_ - int(MARGIN.left() * this->scale_) -
            int(MARGIN.right() * this->scale_) -
            (this->line_ + 1 == MAX_UNCOLLAPSED_LINES ? this->dotdotdotWidth_
                                                      : 0)) -
           this->currentX_;
}

int MessageLayoutContainer::nextWordId()
{
    return this->currentWordId_++;
}

void MessageLayoutContainer::addElement(MessageLayoutElement *element,
                                        const bool forceAdd,
                                        const int prevIndex)
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
        this->currentY_ = int(MARGIN.top() * this->scale_);
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
        yOffset -= (MARGIN.top() * this->scale_);
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
        this->addElement(this->elements_[correctSequence[0]].get(), false, -1);
    }

    for (int i = 1; i < correctSequence.size() && this->canAddElements(); i++)
    {
        this->addElement(this->elements_[correctSequence[i]].get(), false,
                         correctSequence[i - 1]);
    }
}

void MessageLayoutContainer::paintSelectionRect(QPainter &painter,
                                                const Line &line,
                                                const int left, const int right,
                                                const int yOffset,
                                                const QColor &color) const
{
    QRect rect = line.rect;

    rect.setTop(std::max(0, rect.top()) + yOffset);
    rect.setBottom(std::min(this->height_, rect.bottom()) + yOffset);
    rect.setLeft(left);
    rect.setRight(right);

    painter.fillRect(rect, color);
}

std::optional<size_t> MessageLayoutContainer::paintSelectionStart(
    QPainter &painter, const size_t messageIndex, const Selection &selection,
    const int yOffset) const
{
    const auto selectionColor = getTheme()->messages.selection;

    // The selection starts in this message
    for (size_t lineIndex = 0; lineIndex < this->lines_.size(); lineIndex++)
    {
        const Line &line = this->lines_[lineIndex];

        // Selection doesn't start in this line
        if (selection.selectionMin.charIndex >= line.endCharIndex)
        {
            continue;
        }

        if (selection.selectionMin.charIndex == line.endCharIndex - 1)
        {
            // Selection starts at the trailing newline
            // NOTE: Should this be included in the selection? Right now this is
            // painted since it's included in the copy action, but if it's trimmed we
            // should stop painting this
            auto right = this->elements_[line.endIndex - 1]->getRect().right();
            this->paintSelectionRect(painter, line, right, right, yOffset,
                                     selectionColor);
            return std::nullopt;
        }

        int x = this->elements_[line.startIndex]->getRect().left();
        int r = this->elements_[line.endIndex - 1]->getRect().right();

        auto index = line.startCharIndex;
        for (auto i = line.startIndex; i < line.endIndex; i++)
        {
            auto indexCount = this->elements_[i]->getSelectionIndexCount();
            if (index + indexCount <= selection.selectionMin.charIndex)
            {
                index += indexCount;
                continue;
            }

            x = this->elements_[i]->getXFromIndex(
                selection.selectionMin.charIndex - index);

            if (selection.selectionMax.messageIndex == messageIndex &&
                selection.selectionMax.charIndex < line.endCharIndex)
            {
                // The selection ends in the same line it started
                index = line.startCharIndex;
                for (auto elementIdx = line.startIndex;
                     elementIdx < line.endIndex; elementIdx++)
                {
                    auto c =
                        this->elements_[elementIdx]->getSelectionIndexCount();

                    if (index + c > selection.selectionMax.charIndex)
                    {
                        r = this->elements_[elementIdx]->getXFromIndex(
                            selection.selectionMax.charIndex - index);
                        break;
                    }
                    index += c;
                }

                this->paintSelectionRect(painter, line, x, r, yOffset,
                                         selectionColor);

                return std::nullopt;
            }

            // doesn't end in this message -> paint the following lines of this message
            if (selection.selectionMax.messageIndex != messageIndex)
            {
                // The selection does not end in this message
                for (size_t lineIndex2 = lineIndex + 1;
                     lineIndex2 < this->lines_.size(); lineIndex2++)
                {
                    const auto &line2 = this->lines_[lineIndex2];
                    auto left =
                        this->elements_[line2.startIndex]->getRect().left();
                    auto right =
                        this->elements_[line2.endIndex - 1]->getRect().right();

                    this->paintSelectionRect(painter, line2, left, right,
                                             yOffset, selectionColor);
                }

                this->paintSelectionRect(painter, line, x, r, yOffset,
                                         selectionColor);

                return std::nullopt;
            }

            // The selection starts in this line, but ends in some next line or message
            this->paintSelectionRect(painter, line, x, r, yOffset,
                                     selectionColor);

            return {++lineIndex};
        }
    }

    return std::nullopt;
}

void MessageLayoutContainer::paintSelectionEnd(QPainter &painter,
                                               size_t lineIndex,
                                               const Selection &selection,
                                               const int yOffset) const
{
    const auto selectionColor = getTheme()->messages.selection;
    // [2] selection contains or ends in this message (starts before our message or line)
    for (; lineIndex < this->lines_.size(); lineIndex++)
    {
        const Line &line = this->lines_[lineIndex];
        size_t index = line.startCharIndex;

        // the whole line is included
        if (line.endCharIndex < selection.selectionMax.charIndex)
        {
            auto left = this->elements_[line.startIndex]->getRect().left();
            auto right = this->elements_[line.endIndex - 1]->getRect().right();
            this->paintSelectionRect(painter, line, left, right, yOffset,
                                     selectionColor);
            continue;
        }

        // find the right end of the selection
        int r = this->elements_[line.endIndex - 1]->getRect().right();

        for (auto i = line.startIndex; i < line.endIndex; i++)
        {
            size_t c = this->elements_[i]->getSelectionIndexCount();

            if (index + c > selection.selectionMax.charIndex)
            {
                r = this->elements_[i]->getXFromIndex(
                    selection.selectionMax.charIndex - index);
                break;
            }

            index += c;
        }

        auto left = this->elements_[line.startIndex]->getRect().left();
        this->paintSelectionRect(painter, line, left, r, yOffset,
                                 selectionColor);

        return;
    }
}

bool MessageLayoutContainer::canAddElements() const
{
    return this->canAddMessages_;
}

bool MessageLayoutContainer::canCollapse() const
{
    return getSettings()->collpseMessagesMinLines.getValue() > 0 &&
           this->flags_.has(MessageFlag::Collapsed);
}

}  // namespace chatterino
