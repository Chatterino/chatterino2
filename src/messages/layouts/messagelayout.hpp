#pragma once

#include "messages/layouts/messagelayoutcontainer.hpp"
#include "messages/layouts/messagelayoutelement.hpp"
#include "messages/message.hpp"
#include "messages/selection.hpp"

#include <QPixmap>

#include <boost/noncopyable.hpp>
#include <cinttypes>
#include <memory>

namespace chatterino {
namespace messages {
namespace layouts {

class MessageLayout;
typedef std::shared_ptr<MessageLayout> MessageLayoutPtr;
typedef uint8_t MessageLayoutFlagsType;

class MessageLayout : boost::noncopyable
{
public:
    enum Flags : MessageLayoutFlagsType { Collapsed, RequiresBufferUpdate, RequiresLayout };

    MessageLayout(MessagePtr message);

    Message *getMessage();

    // Height
    int getHeight() const;

    // Flags
    Flags getFlags() const;
    bool hasFlags(Flags flags) const;
    void addFlags(Flags flags);
    void removeFlags(Flags flags);

    // Layout
    bool layout(int width, float scale, MessageElement::Flags flags);

    // Painting
    void paint(QPainter &painter, int y, int messageIndex, Selection &selection);
    void invalidateBuffer();
    void deleteBuffer();

    // Elements
    const MessageLayoutElement *getElementAt(QPoint point);
    int getLastCharacterIndex() const;
    int getSelectionIndex(QPoint position);
    void addSelectionText(QString &str, int from, int to);

    // Misc
    bool isDisabled() const;

private:
    // variables
    MessagePtr message;
    MessageLayoutContainer container;
    std::shared_ptr<QPixmap> buffer = nullptr;
    bool bufferValid = false;
    Flags flags;

    int height = 0;

    int currentLayoutWidth = -1;
    int fontGeneration = -1;
    int emoteGeneration = -1;
    QString timestampFormat;
    float scale = -1;
    unsigned int bufferUpdatedCount = 0;

    MessageElement::Flags currentWordFlags = MessageElement::None;

    int collapsedHeight = 32;

    // methods
    void actuallyLayout(int width, MessageElement::Flags flags);
    void updateBuffer(QPixmap *pixmap, int messageIndex, Selection &selection);
};

}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
