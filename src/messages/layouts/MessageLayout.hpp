#pragma once

#include "common/Common.hpp"
#include "common/FlagsEnum.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"

#include <QPixmap>

#include <cinttypes>
#include <memory>

namespace chatterino {

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

struct Selection;
struct MessageLayoutContainer;
class MessageLayoutElement;
struct MessagePaintContext;
struct MessageLayoutContext;

enum class MessageElementFlag : int64_t;
using MessageElementFlags = FlagsEnum<MessageElementFlag>;

enum class MessageLayoutFlag : uint8_t {
    RequiresBufferUpdate = 1 << 1,
    RequiresLayout = 1 << 2,
    AlternateBackground = 1 << 3,
    Collapsed = 1 << 4,
    Expanded = 1 << 5,
    IgnoreHighlights = 1 << 6,
};
using MessageLayoutFlags = FlagsEnum<MessageLayoutFlag>;

struct MessagePaintResult {
    bool hasAnimatedElements = false;
};

class MessageLayout
{
public:
    MessageLayout(MessagePtr message_);
    ~MessageLayout();

    MessageLayout(const MessageLayout &) = delete;
    MessageLayout &operator=(const MessageLayout &) = delete;

    MessageLayout(MessageLayout &&) = delete;
    MessageLayout &operator=(MessageLayout &&) = delete;

    const Message *getMessage();
    const MessagePtr &getMessagePtr() const;

    int getHeight() const;
    int getWidth() const;

    MessageLayoutFlags flags;

    bool layout(const MessageLayoutContext &ctx, bool shouldInvalidateBuffer);

    // Painting
    MessagePaintResult paint(const MessagePaintContext &ctx);
    void invalidateBuffer();
    void deleteBuffer();
    void deleteCache();

    /**
     * Returns a raw pointer to the element at the given point
     *
     * If no element is found at the given point, this returns a null pointer
     */
    const MessageLayoutElement *getElementAt(QPoint point) const;

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
        const MessageLayoutElement *hoveredElement, QPoint relativePos) const;

    /**
     * Get the index of the last character in this message's container
     * This is the sum of all the characters in `elements_`
     */
    size_t getLastCharacterIndex() const;

    /**
     * Get the index of the first visible character in this message's container
     * This is not always 0 in case there elements that are skipped
     */
    size_t getFirstMessageCharacterIndex() const;

    /**
     * Get the character index at the given position, in the context of selections
     */
    size_t getSelectionIndex(QPoint position) const;
    void addSelectionText(QString &str, uint32_t from = 0,
                          uint32_t to = UINT32_MAX,
                          CopyMode copymode = CopyMode::Everything);

    // Misc
    bool isDisabled() const;
    bool isReplyable() const;

private:
    // methods
    void actuallyLayout(const MessageLayoutContext &ctx);
    void updateBuffer(QPixmap *buffer, const MessagePaintContext &ctx);

    // Create new buffer if required, returning the buffer
    QPixmap *ensureBuffer(QPainter &painter, int width, bool clear);

    // variables
    const MessagePtr message_;
    MessageLayoutContainer container_;
    std::unique_ptr<QPixmap> buffer_;
    bool bufferValid_ = false;

    int height_ = 0;
    int currentLayoutWidth_ = -1;
    int layoutState_ = -1;
    float scale_ = -1;
    float imageScale_ = -1.F;
    MessageElementFlags currentWordFlags_;

#ifdef FOURTF
    // Debug counters
    unsigned int layoutCount_ = 0;
    unsigned int bufferUpdatedCount_ = 0;
#endif
};

using MessageLayoutPtr = std::shared_ptr<MessageLayout>;

}  // namespace chatterino
