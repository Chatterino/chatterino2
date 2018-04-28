#include "messages/layouts/messagelayout.hpp"

#include "application.hpp"
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
    util::DebugCount::increase("message layout");
}

MessageLayout::~MessageLayout()
{
    util::DebugCount::decrease("message layout");
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

// Layout
// return true if redraw is required
bool MessageLayout::layout(int width, float scale, MessageElement::Flags flags)
{
    auto app = getApp();

    bool layoutRequired = false;

    // check if width changed
    bool widthChanged = width != this->currentLayoutWidth;
    layoutRequired |= widthChanged;
    this->currentLayoutWidth = width;

    // check if emotes changed
    bool imagesChanged = this->emoteGeneration != app->emotes->getGeneration();
    layoutRequired |= imagesChanged;
    this->emoteGeneration = app->emotes->getGeneration();

    // check if text changed
    bool textChanged =
        this->fontGeneration != singletons::FontManager::getInstance().getGeneration();
    layoutRequired |= textChanged;
    this->fontGeneration = singletons::FontManager::getInstance().getGeneration();

    // check if work mask changed
    bool wordMaskChanged = this->currentWordFlags != flags;  // app->settings->getWordTypeMask();
    layoutRequired |= wordMaskChanged;
    this->currentWordFlags = flags;  // app->settings->getWordTypeMask();

    // check if timestamp format changed
    bool timestampFormatChanged = this->timestampFormat != app->settings->timestampFormat;

    layoutRequired |= timestampFormatChanged;

    // check if dpi changed
    bool scaleChanged = this->scale != scale;
    layoutRequired |= scaleChanged;
    this->scale = scale;
    imagesChanged |= scaleChanged;
    textChanged |= scaleChanged;

    // update word sizes if needed
    if (imagesChanged) {
        //        this->container.updateImages();
        this->flags |= MessageLayout::RequiresBufferUpdate;
    }
    if (textChanged) {
        //        this->container.updateText();
        this->flags |= MessageLayout::RequiresBufferUpdate;
    }
    if (widthChanged || wordMaskChanged) {
        this->deleteBuffer();
    }

    // return if no layout is required
    if (!layoutRequired) {
        return false;
    }

    this->actuallyLayout(width, flags);
    this->invalidateBuffer();

    return true;
}

void MessageLayout::actuallyLayout(int width, MessageElement::Flags flags)
{
    this->container.begin(width, this->scale, this->message->flags.value);

    for (const std::unique_ptr<MessageElement> &element : this->message->getElements()) {
        element->addToContainer(this->container, flags);
    }

    if (this->height != this->container.getHeight()) {
        this->deleteBuffer();
    }

    this->container.end();
    this->height = this->container.getHeight();
}

// Painting
void MessageLayout::paint(QPainter &painter, int y, int messageIndex, Selection &selection,
                          bool isLastReadMessage, bool isWindowFocused)
{
    auto app = getApp();
    QPixmap *pixmap = this->buffer.get();

    // create new buffer if required
    if (!pixmap) {
#ifdef Q_OS_MACOS
        pixmap =
            new QPixmap((int)(this->container.getWidth() * painter.device()->devicePixelRatioF()),
                        (int)(this->container.getHeight() * painter.device()->devicePixelRatioF()));
        pixmap->setDevicePixelRatio(painter.device()->devicePixelRatioF());
#else
        pixmap = new QPixmap(this->container.getWidth(), std::max(16, this->container.getHeight()));
#endif

        this->buffer = std::shared_ptr<QPixmap>(pixmap);
        this->bufferValid = false;
        util::DebugCount::increase("message drawing buffers");
    }

    if (!this->bufferValid || !selection.isEmpty()) {
        this->updateBuffer(pixmap, messageIndex, selection);
    }

    // draw on buffer
    painter.drawPixmap(0, y, *pixmap);
    //    painter.drawPixmap(0, y, this->container.width, this->container.getHeight(), *pixmap);

    // draw gif emotes
    this->container.paintAnimatedElements(painter, y);

    // draw disabled
    if (this->message->flags.HasFlag(Message::Disabled)) {
        painter.fillRect(0, y, pixmap->width(), pixmap->height(),
                         app->themes->messages.disabled);
    }

    // draw selection
    if (!selection.isEmpty()) {
        this->container.paintSelection(painter, messageIndex, selection, y);
    }

    // draw last read message line
    if (isLastReadMessage) {
        QColor color = isWindowFocused
                           ? app->themes->tabs.selected.backgrounds.regular.color()
                           : app->themes->tabs.selected.backgrounds.unfocused.color();

        QBrush brush(color, Qt::VerPattern);

        painter.fillRect(0, y + this->container.getHeight() - 1, this->container.getWidth(), 1,
                         brush);
    }

    this->bufferValid = true;
}

void MessageLayout::updateBuffer(QPixmap *buffer, int messageIndex, Selection &selection)
{
    auto app = getApp();

    QPainter painter(buffer);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw background
    painter.fillRect(buffer->rect(), this->message->flags & Message::Highlighted
                                         ? app->themes->messages.backgrounds.highlighted
                                         : app->themes->messages.backgrounds.regular);

    // draw message
    this->container.paintElements(painter);

#ifdef FOURTF
    // debug
    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(buffer->rect().x(), buffer->rect().y(), buffer->rect().width() - 1,
                     buffer->rect().height() - 1);

    QTextOption option;
    option.setAlignment(Qt::AlignRight | Qt::AlignTop);

    painter.drawText(QRectF(1, 1, this->container.getWidth() - 3, 1000),
                     QString::number(++this->bufferUpdatedCount), option);
#endif
}

void MessageLayout::invalidateBuffer()
{
    this->bufferValid = false;
}

void MessageLayout::deleteBuffer()
{
    if (this->buffer != nullptr) {
        util::DebugCount::decrease("message drawing buffers");

        this->buffer = nullptr;
    }
}

void MessageLayout::deleteCache()
{
    this->deleteBuffer();

#ifdef XD
    this->container.clear();
#endif
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

int MessageLayout::getLastCharacterIndex() const
{
    return this->container.getLastCharacterIndex();
}

int MessageLayout::getSelectionIndex(QPoint position)
{
    return this->container.getSelectionIndex(position);
}

void MessageLayout::addSelectionText(QString &str, int from, int to)
{
    this->container.addSelectionText(str, from, to);
}

}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
