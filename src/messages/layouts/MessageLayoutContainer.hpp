#pragma once

#include "common/Common.hpp"
#include "common/FlagsEnum.hpp"

#include <QPoint>
#include <QRect>

#include <memory>
#include <optional>
#include <vector>

class QPainter;

namespace chatterino {

enum class MessageFlag : int64_t;
enum class FirstWord { Neutral, RTL, LTR };
using MessageFlags = FlagsEnum<MessageFlag>;
class MessageLayoutElement;
struct Selection;
struct MessagePaintContext;

struct MessageLayoutContainer {
    MessageLayoutContainer() = default;

    FirstWord first = FirstWord::Neutral;

    /**
     * Begin the layout process of this message
     *
     * This will reset all line calculations, and will be considered incomplete
     * until the accompanying end function has been called
     */
    void beginLayout(int width_, float scale_, MessageFlags flags_);

    /**
     * Finish the layout process of this message
     */
    void endLayout();

    /**
     * Add the given `element` to this message.
     *
     * This will also prepend a line break if the element
     * does not fit in the current line
     */
    void addElement(MessageLayoutElement *element);

    /**
     * Add the given `element` to this message
     */
    void addElementNoLineBreak(MessageLayoutElement *element);

    /**
     * Break the current line
     */
    void breakLine();

    /**
     * Paint the elements in this message
     */
    void paintElements(QPainter &painter, const MessagePaintContext &ctx) const;

    /**
     * Paint the animated elements in this message
     * @returns true if this container contains at least one animated element
     */
    bool paintAnimatedElements(QPainter &painter, int yOffset) const;

    /**
     * Paint the selection for this container
     * This container contains one or more message elements
     *
     * @param painter The painter we draw everything to
     * @param messageIndex This container's message index in the context of
     *                     the layout we're being painted in
     * @param selection The selection we need to paint
     * @param yOffset The extra offset added to Y for everything that's painted
     */
    void paintSelection(QPainter &painter, size_t messageIndex,
                        const Selection &selection, int yOffset) const;

    /**
     * Add text from this message into the `str` parameter
     *
     * @param[out] str The string where we append our selected text to
     * @param from The character index from which we collecting our selected text
     * @param to The character index where we stop collecting our selected text
     * @param copymode Decides what from the message gets added to the selected text
     */
    void addSelectionText(QString &str, uint32_t from, uint32_t to,
                          CopyMode copymode) const;

    /**
     * Returns a raw pointer to the element at the given point
     *
     * If no element is found at the given point, this returns a null pointer
     */
    MessageLayoutElement *getElementAt(QPoint point) const;

    /**
     * Get the character index at the given point, in the context of selections
     */
    size_t getSelectionIndex(QPoint point) const;

    /**
     * Get the index of the first visible character in this message
     *
     * This can be non-zero if there are elements in this message that are skipped
     */
    size_t getFirstMessageCharacterIndex() const;

    /**
     * @brief Returns the word bounds of the given element
     *
     * The first value is the index of the first character in the word,
     * the second value is the index of the character after the last character in the word.
     *
     * Given the word "abc" by itself, we would return (0, 3)
     *
     *  V  V
     * "abc "
     */
    std::pair<int, int> getWordBounds(
        const MessageLayoutElement *hoveredElement) const;

    /**
     * Get the index of the last character in this message
     * This is the sum of all the characters in `elements_`
     */
    size_t getLastCharacterIndex() const;

    /**
     * Returns the width of this message
     */
    int getWidth() const;

    /**
     * Returns the height of this message
     */
    int getHeight() const;

    /**
     * Returns the scale of this message
     */
    float getScale() const;

    /**
     * Returns true if this message is collapsed
     */
    bool isCollapsed() const;

    /**
     * Return true if we are at the start of a new line
     */
    bool atStartOfLine() const;

    /**
     * Check whether an additional `width` would fit in the current line
     *
     * Returns true if it does fit, false if not
     */
    bool fitsInLine(int width) const;

    /**
     * Returns the remaining width of this line until we will need to start a new line
     */
    int remainingWidth() const;

    /**
     * Returns the id of the next word that can be added to this container
     */
    int nextWordId();

private:
    struct Line {
        /**
         * The index of the first message element on this line
         * Points into `elements_`
         */
        size_t startIndex{};

        /**
         * The index of the last message element on this line
         * Points into `elements_`
         */
        size_t endIndex{};

        /**
         * In the context of selections, the index of the first character on this line
         * The first line's startCharIndex will always be 0
         */
        size_t startCharIndex{};

        /**
         * In the context of selections, the index of the last character on this line
         * The last line's startCharIndex will always be the sum of all characters in this message
         */
        size_t endCharIndex{};

        /**
         * The rectangle that covers all elements on this line
         * This rectangle will always take up 100% of the view's width
         */
        QRect rect;
    };

    /*
    addElement is called at two stages. first stage is the normal one where we want to add message layout elements to the container.
    If we detect an RTL word in the message, reorderRTL will be called, which is the second stage, where we call _addElement
    again for each layout element, but in the correct order this time, without adding the elemnt to the this->element_ vector.
    Due to compact emote logic, we need the previous element to check if we should change the spacing or not.
    in stage one, this is simply elements_.back(), but in stage 2 that's not the case due to the reordering, and we need to pass the 
    index of the reordered previous element. 
    In stage one we don't need that and we pass -2 to indicate stage one (i.e. adding mode)
    In stage two, we pass -1 for the first element, and the index of the oredered privous element for the rest.
    */
    void addElement(MessageLayoutElement *element, bool forceAdd,
                    int prevIndex);

    // this method is called when a message has an RTL word
    // we need to reorder the words to be shown properly
    // however we don't we to reorder non-text elements like badges, timestamps, username
    // firstTextIndex is the index of the first text element that we need to start the reordering from
    void reorderRTL(int firstTextIndex);

    /**
     * Paint a selection rectangle over the given line
     *
     * @param painter The painter we draw everything to
     * @param line The line whose rect we use as the base top & bottom of the rect to paint
     * @param left The left coordinates of the rect to paint
     * @param right The right coordinates of the rect to paint
     * @param yOffset Extra offset for line's top & bottom
     * @param color Color of the selection
     **/
    void paintSelectionRect(QPainter &painter, const Line &line, int left,
                            int right, int yOffset, const QColor &color) const;

    /**
     * Paint the selection start
     *
     * Returns a line index if this message should also paint the selection end
     */
    std::optional<size_t> paintSelectionStart(QPainter &painter,
                                              size_t messageIndex,
                                              const Selection &selection,
                                              int yOffset) const;

    /**
     * Paint the selection end
     *
     * @param lineIndex The index of the line to start painting at
     */
    void paintSelectionEnd(QPainter &painter, size_t lineIndex,
                           const Selection &selection, int yOffset) const;

    /**
     * canAddElements returns true if it's possible to add more elements to this message
     */
    bool canAddElements() const;

    /**
     * Return true if this message can collapse
     *
     * TODO: comment this better :-)
     */
    bool canCollapse() const;

    // variables
    float scale_ = 1.F;
    int width_ = 0;
    MessageFlags flags_{};
    /**
     * line_ is the current line index we are adding
     * This is not the number of lines this message contains, since this will stop
     * incrementing if the message is collapsed
     */
    size_t line_{};
    int height_ = 0;
    int currentX_ = 0;
    int currentY_ = 0;
    /**
     * charIndex_ is the selection-contexted index of where we currently are in our message
     * At the end, this will always be equal to the sum of `elements_` getSelectionIndexCount()
     */
    size_t charIndex_ = 0;
    size_t lineStart_ = 0;
    int lineHeight_ = 0;
    int spaceWidth_ = 4;
    int textLineHeight_ = 0;
    int dotdotdotWidth_ = 0;
    int currentWordId_ = 0;
    bool canAddMessages_ = true;
    bool isCollapsed_ = false;
    bool wasPrevReversed_ = false;

    /**
     * containsRTL indicates whether or not any of the text in this message
     * contains any right-to-left characters (e.g. arabic)
     */
    bool containsRTL = false;

    std::vector<std::unique_ptr<MessageLayoutElement>> elements_;

    /**
     * A list of lines covering this message
     * A message that spans 3 lines in a view will have 3 elements in lines_
     * These lines hold no relation to the elements that are in this
     */
    std::vector<Line> lines_;
};

}  // namespace chatterino
