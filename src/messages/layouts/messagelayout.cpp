#include "messages/layouts/messagelayout.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/settingsmanager.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QThread>
#include <QtGlobal>

#define MARGIN_LEFT (int)(8 * this->scale)
#define MARGIN_RIGHT (int)(8 * this->scale)
#define MARGIN_TOP (int)(4 * this->scale)
#define MARGIN_BOTTOM (int)(4 * this->scale)
#define COMPACT_EMOTES_OFFSET 6

namespace chatterino {
namespace messages {
namespace layouts {

MessageLayout::MessageLayout(MessagePtr _message)
    : message(_message)
    , buffer(nullptr)
{
    if (_message->hasFlags(Message::Collapsed)) {
        this->addFlags(MessageLayout::Collapsed);
    }
}

Message *MessageLayout::getMessage()
{
    return this->message.get();
}

// Height
int MessageLayout::getHeight() const
{
    return container.getHeight();
}

// Flags
MessageLayout::Flags MessageLayout::getFlags() const
{
    return this->flags;
}

bool MessageLayout::hasFlags(Flags _flags) const
{
    return this->flags & _flags;
}

void MessageLayout::addFlags(Flags _flags)
{
    this->flags = (Flags)((MessageLayoutFlagsType)this->flags | (MessageLayoutFlagsType)_flags);
}

void MessageLayout::removeFlags(Flags _flags)
{
    this->flags = (Flags)((MessageLayoutFlagsType)this->flags & ~((MessageLayoutFlagsType)_flags));
}

// Layout
// return true if redraw is required
bool MessageLayout::layout(int width, float scale)
{
    auto &emoteManager = singletons::EmoteManager::getInstance();

    bool layoutRequired = false;

    // check if width changed
    bool widthChanged = width != this->currentLayoutWidth;
    layoutRequired |= widthChanged;
    this->currentLayoutWidth = width;

    // check if emotes changed
    bool imagesChanged = this->emoteGeneration != emoteManager.getGeneration();
    layoutRequired |= imagesChanged;
    this->emoteGeneration = emoteManager.getGeneration();

    // check if text changed
    bool textChanged =
        this->fontGeneration != singletons::FontManager::getInstance().getGeneration();
    layoutRequired |= textChanged;
    this->fontGeneration = singletons::FontManager::getInstance().getGeneration();

    // check if work mask changed
    bool wordMaskChanged =
        this->currentWordTypes != singletons::SettingManager::getInstance().getWordTypeMask();
    layoutRequired |= wordMaskChanged;
    this->currentWordTypes = singletons::SettingManager::getInstance().getWordTypeMask();

    // check if dpi changed
    bool scaleChanged = this->scale != scale;
    layoutRequired |= scaleChanged;
    this->scale = scale;
    imagesChanged |= scaleChanged;
    textChanged |= scaleChanged;

    // update word sizes if needed
    if (imagesChanged) {
        // fourtf: update images
        this->addFlags(MessageLayout::RequiresBufferUpdate);
    }
    if (textChanged) {
        // fourtf: update text
        this->addFlags(MessageLayout::RequiresBufferUpdate);
    }
    if (widthChanged || wordMaskChanged) {
        this->deleteBuffer();
    }

    // return if no layout is required
    if (!layoutRequired) {
        return false;
    }

    this->actuallyLayout(width);
    this->invalidateBuffer();

    return true;
}

void MessageLayout::actuallyLayout(int width)
{
    this->container.clear();
    this->container.width = width;
    this->container.scale = this->scale;

    for (const std::unique_ptr<MessageElement> &element : this->message->getElements()) {
        element->addToContainer(this->container, MessageElement::Default);
    }

    if (this->height != this->container.getHeight()) {
        this->deleteBuffer();
    }

    this->container.finish();
    this->height = this->container.getHeight();
}

// Painting
void MessageLayout::paint(QPainter &painter, int y, int messageIndex, Selection &selection)
{
    QPixmap *pixmap = this->buffer.get();
    singletons::ThemeManager &themeManager = singletons::ThemeManager::getInstance();

    // create new buffer if required
    if (!pixmap) {
#ifdef Q_OS_MACOS

        pixmap =
            new QPixmap((int)(this->container.getWidth() * painter.device()->devicePixelRatioF()),
                        (int)(this->container.getHeight() * painter.device()->devicePixelRatioF()));
        pixmap->setDevicePixelRatio(painter.device()->devicePixelRatioF());
#else
        pixmap = new QPixmap(this->container.width, std::max(16, this->container.getHeight()));
#endif

        this->buffer = std::shared_ptr<QPixmap>(pixmap);
        this->bufferValid = false;
    }

    if (!this->bufferValid) {
        this->updateBuffer(pixmap, messageIndex, selection);
    }

    // draw on buffer
    painter.drawPixmap(0, y, pixmap->width(), pixmap->height(), *pixmap);

    // draw disabled
    if (this->message->hasFlags(Message::Disabled)) {
        painter.fillRect(0, y, pixmap->width(), pixmap->height(), themeManager.messages.disabled);
    }

    // draw gif emotes
    this->container.paintAnimatedElements(painter, y);

    this->bufferValid = true;
}

void MessageLayout::updateBuffer(QPixmap *buffer, int messageIndex, Selection &selection)
{
    singletons::ThemeManager &themeManager = singletons::ThemeManager::getInstance();

    QPainter painter(buffer);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw background
    painter.fillRect(buffer->rect(), this->message->hasFlags(Message::Highlighted)
                                         ? themeManager.messages.backgrounds.highlighted
                                         : themeManager.messages.backgrounds.regular);

    // draw selection
    if (!selection.isEmpty()) {
        this->container.paintSelection(painter, messageIndex, selection);
    }

    // draw message
    this->container.paintElements(painter);

#ifdef OHHEYITSFOURTF
    // debug
    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(buffer->rect().x(), buffer->rect().y(), buffer->rect().width() - 1,
                     buffer->rect().height() - 1);

    QTextOption option;
    option.setAlignment(Qt::AlignRight | Qt::AlignTop);

    painter.drawText(QRectF(1, 1, this->container.width - 3, 1000),
                     QString::number(++this->bufferUpdatedCount), option);
#endif
}

void MessageLayout::invalidateBuffer()
{
    this->bufferValid = false;
}

void MessageLayout::deleteBuffer()
{
    this->buffer = nullptr;
}

// Elements
//    assert(QThread::currentThread() == QApplication::instance()->thread());

// returns nullptr if none was found

// fourtf: this should return a MessageLayoutItem
const MessageLayoutElement *MessageLayout::getElementAt(QPoint point)
{
    // go through all words and return the first one that contains the point.
    return this->container.getElementAt(point);
}

// XXX(pajlada): This is probably not the optimal way to calculate this
int MessageLayout::getLastCharacterIndex() const
{
    // fourtf: xD
    //    // find out in which line the cursor is
    //    int lineNumber = 0, lineStart = 0, lineEnd = 0;

    //    for (size_t i = 0; i < this->wordParts.size(); i++) {
    //        const LayoutItem &part = this->wordParts[i];

    //        if (part.getLineNumber() != lineNumber) {
    //            lineStart = i;
    //            lineNumber = part.getLineNumber();
    //        }

    //        lineEnd = i + 1;
    //    }

    //    // count up to the cursor
    //    int index = 0;

    //    for (int i = 0; i < lineStart; i++) {
    //        const LayoutItem &part = this->wordParts[i];

    //        index += part.getWord().isImage() ? 2 : part.getText().length() + 1;
    //    }

    //    for (int i = lineStart; i < lineEnd; i++) {
    //        const LayoutItem &part = this->wordParts[i];

    //        index += part.getCharacterLength();
    //    }

    //    return index;

    return 0;
}

int MessageLayout::getSelectionIndex(QPoint position)
{
    //    if (this->wordParts.size() == 0) {
    //        return 0;
    //    }

    //    // find out in which line the cursor is
    //    int lineNumber = 0, lineStart = 0, lineEnd = 0;

    //    for (size_t i = 0; i < this->wordParts.size(); i++) {
    //        LayoutItem &part = this->wordParts[i];

    //        if (part.getLineNumber() != 0 && position.y() < part.getY()) {
    //            break;
    //        }

    //        if (part.getLineNumber() != lineNumber) {
    //            lineStart = i;
    //            lineNumber = part.getLineNumber();
    //        }

    //        lineEnd = i + 1;
    //    }

    //    // count up to the cursor
    //    int index = 0;

    //    for (int i = 0; i < lineStart; i++) {
    //        LayoutItem &part = this->wordParts[i];

    //        index += part.getWord().isImage() ? 2 : part.getText().length() + 1;
    //    }

    //    for (int i = lineStart; i < lineEnd; i++) {
    //        LayoutItem &part = this->wordParts[i];

    //        // curser is left of the word part
    //        if (position.x() < part.getX()) {
    //            break;
    //        }

    //        // cursor is right of the word part
    //        if (position.x() > part.getX() + part.getWidth()) {
    //            index += part.getCharacterLength();
    //            continue;
    //        }

    //        // cursor is over the word part
    //        if (part.getWord().isImage()) {
    //            if (position.x() - part.getX() > part.getWidth() / 2) {
    //                index++;
    //            }
    //        } else {
    //            // TODO: use word.getCharacterWidthCache();

    //            auto text = part.getText();

    //            int x = part.getX();

    //            for (int j = 0; j < text.length(); j++) {
    //                if (x > position.x()) {
    //                    break;
    //                }

    //                index++;
    //                x = part.getX() + part.getWord().getFontMetrics(this->scale).width(text, j +
    //                1);
    //            }
    //        }

    //        break;
    //    }

    //    return index;

    return 0;
}
}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
