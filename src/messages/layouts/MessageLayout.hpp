#pragma once

#include "common/Common.hpp"
#include "common/FlagsEnum.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"

#include <boost/noncopyable.hpp>
#include <QPixmap>

#include <cinttypes>
#include <memory>

namespace chatterino {

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

struct Selection;
struct MessageLayoutContainer;
class MessageLayoutElement;

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

class MessageLayout : boost::noncopyable
{
public:
    MessageLayout(MessagePtr message_);
    ~MessageLayout();

    const Message *getMessage();
    const MessagePtr &getMessagePtr() const;

    int getHeight() const;
    int getWidth() const;

    MessageLayoutFlags flags;

    bool layout(int width, float scale_, MessageElementFlags flags);

    // Painting
    void paint(QPainter &painter, int width, int y, int messageIndex,
               Selection &selection, bool isLastReadMessage,
               bool isWindowFocused, bool isMentions);
    void invalidateBuffer();
    void deleteBuffer();
    void deleteCache();

    // Elements
    const MessageLayoutElement *getElementAt(QPoint point);
    int getLastCharacterIndex() const;
    int getFirstMessageCharacterIndex() const;
    int getSelectionIndex(QPoint position);
    void addSelectionText(QString &str, uint32_t from = 0,
                          uint32_t to = UINT32_MAX,
                          CopyMode copymode = CopyMode::Everything);

    // Misc
    bool isDisabled() const;
    bool isReplyable() const;

private:
    // methods
    void actuallyLayout(int width, MessageElementFlags flags);
    void updateBuffer(QPixmap *pixmap, int messageIndex, Selection &selection);

    // Create new buffer if required, returning the buffer
    QPixmap *ensureBuffer(QPainter &painter, int width);

    // variables
    MessagePtr message_;
    MessageLayoutContainer container_;
    std::unique_ptr<QPixmap> buffer_{};
    bool bufferValid_ = false;

    int height_ = 0;
    int currentLayoutWidth_ = -1;
    int layoutState_ = -1;
    float scale_ = -1;
    MessageElementFlags currentWordFlags_;

#ifdef FOURTF
    // Debug counters
    unsigned int layoutCount_ = 0;
    unsigned int bufferUpdatedCount_ = 0;
#endif
};

using MessageLayoutPtr = std::shared_ptr<MessageLayout>;

}  // namespace chatterino
