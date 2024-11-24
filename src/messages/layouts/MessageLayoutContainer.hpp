#pragma once

#include "common/Common.hpp"
#include "common/FlagsEnum.hpp"
#include "messages/MessageFlag.hpp"

#include <QPoint>
#include <QRect>

#include <memory>
#include <optional>
#include <vector>

#if __has_include(<gtest/gtest_prod.h>)
#    include <gtest/gtest_prod.h>
#endif

class QPainter;

namespace chatterino {

enum class TextDirection : uint8_t {
    Neutral,
    RTL,
    LTR,
};

class MessageLayoutElement;
struct Selection;
struct MessagePaintContext;

struct MessageLayoutContainer {
    MessageLayoutContainer() = default;

    /**
     * Begin the layout process of this message
     *
     * This will reset all line calculations, and will be considered incomplete
     * until the accompanying end function has been called
     */
    void beginLayout(int width, float scale, float imageScale,
                     MessageFlags flags);

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
     * Returns the image scale
     */
    float getImageScale() const;

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

    /// @brief Attempts to add @a element to this container
    ///
    /// This can be called in two scenarios.
    ///
    /// 1. **Regular**: In this scenario, @a element is positioned and added
    ///    to the internal container.
    ///    This is active iff @a prevIndex is `-2`.
    ///    During this stage, if there isn't any @a textDirection_ detected yet,
    ///    the added element is checked if it contains RTL/LTR text to infer the
    ///    direction. Only upon calling @a breakLine, the elements will be
    ///    visually reorderd.
    ///
    /// 2. **Repositioning**: In this scenario, @a element is already added to
    ///    the container thus it's only repositioned.
    ///    This is active iff @a prevIndex is not `-2`.
    ///    @a prevIndex is used to handle compact emotes. `-1` is used to
    ///    indicate no predecessor.
    ///
    /// @param element[in] The element to add. This must be non-null and
    ///                    allocated with `new`. Ownership is transferred
    ///                    into this container.
    /// @param forceAdd When enabled, @a element will be added regardless of
    ///                 `canAddElements`. If @a element won't be added it will
    ///                 be `delete`d.
    /// @param prevIndex Controls the "scenario" (see above). `-2` indicates
    ///                  "regular" mode; other values indicate "repositioning".
    ///                  In case of repositioning, this contains the index of
    ///                  the precesding element (visually, according to
    ///                  @a textDirection_ [RTL/LTR]).
    void addElement(MessageLayoutElement *element, bool forceAdd,
                    qsizetype prevIndex);

    /// @brief Reorders the last line according to @a textDirection_
    ///
    /// If a line contains RTL or the text direction is RTL, elements need to be
    /// reordered (see @a lineContainsRTL_ and @a isRTL respectively).
    /// This method reverses sequences of text in the opposite direction for it
    /// to remain in its intended direction when rendered. Non-text elements
    /// won't be reordered.
    ///
    /// For example, in an RTL container, the sequence
    /// "1_R 2_R 3_N 4_R 5_L 6_L 7_N 8_R" will be (visually) reordered to
    /// "8_R 5_L 6_L 7_N 4_R 3_N 2_R 1_R" (x_{L,N,R} indicates the element with
    /// id x which is in the direction {LTR,Neutral,RTL}).
    ///
    /// @param firstTextIndex The index of the first element of the message
    ///                       (i.e. the index after the username).
    void reorderRTL(size_t firstTextIndex);

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
     * @returns A line index if the selection ends within this message but start
     *          and end are on different lines. The returned index is the index 
     *          of the first line where the selection starts at the beginning of
     *          the line. This index should be passed to paintSelectionEnd().
     *          If `std::nullopt` is returned, no further call to 
     *          paintSelectionEnd() is necessary.
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

    /// @returns true, if @a textDirection_ is RTL
    [[nodiscard]] bool isRTL() const noexcept;

    /// @returns true, if @a textDirection_ is LTR
    [[nodiscard]] bool isLTR() const noexcept;

    /// @returns true, if @a textDirection_ is Neutral
    [[nodiscard]] bool isNeutral() const noexcept;

    // variables
    float scale_ = 1.F;
    /**
     * Scale factor for images
     */
    float imageScale_ = 1.F;
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

    /// @brief True if the current line contains any RTL text.
    ///
    /// If the line contains any RTL, it needs to be reordered after a
    /// linebreak after which it's reset to `false`.
    bool lineContainsRTL_ = false;

    /// True if there was any RTL/LTR reordering done in this container
    bool anyReorderingDone_ = false;

    /// @brief The direction of the text in this container.
    ///
    /// This starts off as neutral until an element is encountered that is
    /// either LTR or RTL (afterwards this remains constant).
    TextDirection textDirection_ = TextDirection::Neutral;

    std::vector<std::unique_ptr<MessageLayoutElement>> elements_;

    /**
     * A list of lines covering this message
     * A message that spans 3 lines in a view will have 3 elements in lines_
     * These lines hold no relation to the elements that are in this
     */
    std::vector<Line> lines_;

#ifdef FRIEND_TEST
    FRIEND_TEST(MessageLayoutContainerTest, RtlReordering);
#endif
};

}  // namespace chatterino
