#pragma once

#include "messages/layouts/messagelayoutcontainer.hpp"
#include "messages/layouts/messagelayoutelement.hpp"
#include "messages/message.hpp"
#include "messages/selection.hpp"
#include "util/flagsenum.hpp"

#include <QPixmap>

#include <boost/noncopyable.hpp>
#include <cinttypes>
#include <memory>

namespace chatterino {
namespace messages {
namespace layouts {

class MessageLayout : boost::noncopyable
{
public:
    enum Flags : uint8_t {
        RequiresBufferUpdate = 1 << 1,
        RequiresLayout = 1 << 2,
        AlternateBackground = 1 << 3,
        Collapsed = 1 << 4,
        Expanded = 1 << 5,
    };

    MessageLayout(MessagePtr m_message);
    ~MessageLayout();

    Message *getMessage();

    // Height
    int getHeight() const;

    // Flags
    util::FlagsEnum<Flags> flags;

    // Layout
    bool layout(int width, float m_scale, MessageElement::Flags flags);

    // Painting
    void paint(QPainter &painter, int width, int y, int messageIndex, Selection &selection,
               bool isLastReadMessage, bool isWindowFocused);
    void invalidateBuffer();
    void deleteBuffer();
    void deleteCache();

    // Elements
    const MessageLayoutElement *getElementAt(QPoint point);
    int getLastCharacterIndex() const;
    int getSelectionIndex(QPoint position);
    void addSelectionText(QString &str, int from = 0, int to = INT_MAX);

    // Misc
    bool isDisabled() const;

private:
    // variables
    MessagePtr m_message;
    MessageLayoutContainer m_container;
    std::shared_ptr<QPixmap> m_buffer = nullptr;
    bool m_bufferValid = false;

    int m_height = 0;

    int m_currentLayoutWidth = -1;
    int m_fontGeneration = -1;
    int m_emoteGeneration = -1;
    QString m_timestampFormat;
    float m_scale = -1;
    unsigned int m_bufferUpdatedCount = 0;

    MessageElement::Flags m_currentWordFlags = MessageElement::None;

    int m_collapsedHeight = 32;

    // methods
    void actuallyLayout(int width, MessageElement::Flags flags);
    void updateBuffer(QPixmap *pixmap, int messageIndex, Selection &selection);
};

using MessageLayoutPtr = std::shared_ptr<MessageLayout>;

}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
