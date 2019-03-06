#pragma once

#include "messages/Common.hpp"
#include "messages/ThemexD.hpp"
#include "util/FlagsEnum.hpp"

#include <QPixmap>
#include <boost/noncopyable.hpp>
#include <cinttypes>
#include <memory>

namespace chatterino
{
    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    struct Selection;
    struct MessageLayoutContainer;
    class MessageLayoutElement;

    enum class MessageElementFlag;
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
        MessageLayout(MessagePtr message);
        ~MessageLayout();

        const Message* getMessage();

        int getHeight() const;

        MessageLayoutFlags flags;

        bool layout(
            ThemexD& theme, int width, float scale_, MessageElementFlags flags);

        // Painting
        void paint(ThemexD& theme, QPainter& painter, int width, int y,
            int messageIndex, const Selection& selection,
            bool isLastReadMessage, bool isWindowFocused);
        void invalidateBuffer();
        void deleteBuffer();
        void deleteCache();

        // Elements
        const MessageLayoutElement* getElementAt(QPoint point);
        int getLastCharacterIndex() const;
        int getFirstMessageCharacterIndex() const;
        int getSelectionIndex(QPoint position);
        void addSelectionText(QString& str, int from = 0, int to = INT_MAX,
            CopyMode copymode = CopyMode::Everything);

        // Misc
        bool isDisabled() const;

    private:
        // variables
        MessagePtr message_;
        std::shared_ptr<MessageLayoutContainer> container_;
        std::shared_ptr<QPixmap> buffer_{};
        bool bufferValid_ = false;

        int height_ = 0;

        int currentLayoutWidth_ = -1;
        int layoutState_ = -1;
        int imageState_ = -1;
        float scale_ = -1;
        unsigned int layoutCount_ = 0;
        unsigned int bufferUpdatedCount_ = 0;

        MessageElementFlags currentWordFlags_;

        int collapsedHeight_ = 32;

        // methods
        void actuallyLayout(
            ThemexD& theme, int width, MessageElementFlags flags);
        void updateBuffer(ThemexD& theme, QPixmap* pixmap, int messageIndex,
            const Selection& selection);
    };

    using MessageLayoutPtr = std::shared_ptr<MessageLayout>;

}  // namespace chatterino
