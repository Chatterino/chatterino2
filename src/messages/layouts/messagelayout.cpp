#include "messages/layouts/messagelayout.hpp"

#include "application.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/benchmark.hpp"

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

MessageLayout::MessageLayout(MessagePtr message)
    : m_message(message)
    , m_buffer(nullptr)
{
    util::DebugCount::increase("message layout");
}

MessageLayout::~MessageLayout()
{
    util::DebugCount::decrease("message layout");
}

Message *MessageLayout::getMessage()
{
    return this->m_message.get();
}

// Height
int MessageLayout::getHeight() const
{
    return m_container.getHeight();
}

// Layout
// return true if redraw is required
bool MessageLayout::layout(int width, float scale, MessageElement::Flags flags)
{
    BenchmarkGuard benchmark("MessageLayout::layout()");

    auto app = getApp();

    bool layoutRequired = false;

    // check if width changed
    bool widthChanged = width != this->m_currentLayoutWidth;
    layoutRequired |= widthChanged;
    this->m_currentLayoutWidth = width;

    // check if emotes changed
    bool imagesChanged = this->m_emoteGeneration != app->emotes->getGeneration();
    layoutRequired |= imagesChanged;
    this->m_emoteGeneration = app->emotes->getGeneration();

    // check if text changed
    bool textChanged = this->m_fontGeneration != app->fonts->getGeneration();
    layoutRequired |= textChanged;
    this->m_fontGeneration = app->fonts->getGeneration();

    // check if work mask changed
    bool wordMaskChanged = this->m_currentWordFlags != flags;  // app->settings->getWordTypeMask();
    layoutRequired |= wordMaskChanged;
    this->m_currentWordFlags = flags;  // app->settings->getWordTypeMask();

    // check if timestamp format changed
    bool timestampFormatChanged = this->m_timestampFormat != app->settings->timestampFormat;
    this->m_timestampFormat = app->settings->timestampFormat.getValue();

    layoutRequired |= timestampFormatChanged;

    // check if layout was requested manually
    layoutRequired |= bool(this->flags & RequiresLayout);
    this->flags &= ~RequiresLayout;

    // check if dpi changed
    bool scaleChanged = this->m_scale != scale;
    layoutRequired |= scaleChanged;
    this->m_scale = scale;
    imagesChanged |= scaleChanged;
    textChanged |= scaleChanged;

    //    assert(layoutRequired);

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
    qDebug() << layoutRequired;

    if (!layoutRequired) {
        return false;
    }

    this->actuallyLayout(width, flags);
    this->invalidateBuffer();

    return true;
}

void MessageLayout::actuallyLayout(int width, MessageElement::Flags _flags)
{
    auto messageFlags = this->m_message->flags.value;

    if (this->flags & MessageLayout::Expanded ||
        (_flags & MessageElement::ModeratorTools &&
         !(this->m_message->flags & Message::MessageFlags::Disabled))) {
        messageFlags = Message::MessageFlags(messageFlags & ~Message::MessageFlags::Collapsed);
    }

    this->m_container.begin(width, this->m_scale, messageFlags);

    for (const std::unique_ptr<MessageElement> &element : this->m_message->getElements()) {
        element->addToContainer(this->m_container, _flags);
    }

    if (this->m_height != this->m_container.getHeight()) {
        this->deleteBuffer();
    }

    this->m_container.end();
    this->m_height = this->m_container.getHeight();

    // collapsed state
    this->flags &= ~Flags::Collapsed;
    if (this->m_container.isCollapsed()) {
        this->flags |= Flags::Collapsed;
    }
}

// Painting
void MessageLayout::paint(QPainter &painter, int y, int messageIndex, Selection &selection,
                          bool isLastReadMessage, bool isWindowFocused)
{
    auto app = getApp();
    QPixmap *pixmap = this->m_buffer.get();

    // create new buffer if required
    if (!pixmap) {
#ifdef Q_OS_MACOS
        pixmap =
            new QPixmap((int)(this->container.getWidth() * painter.device()->devicePixelRatioF()),
                        (int)(this->container.getHeight() * painter.device()->devicePixelRatioF()));
        pixmap->setDevicePixelRatio(painter.device()->devicePixelRatioF());
#else
        pixmap =
            new QPixmap(this->m_container.getWidth(), std::max(16, this->m_container.getHeight()));
#endif

        this->m_buffer = std::shared_ptr<QPixmap>(pixmap);
        this->m_bufferValid = false;
        util::DebugCount::increase("message drawing buffers");
    }

    if (!this->m_bufferValid || !selection.isEmpty()) {
        this->updateBuffer(pixmap, messageIndex, selection);
    }

    // draw on buffer
    painter.drawPixmap(0, y, *pixmap);
    //    painter.drawPixmap(0, y, this->container.width, this->container.getHeight(), *pixmap);

    // draw gif emotes
    this->m_container.paintAnimatedElements(painter, y);

    // draw disabled
    if (this->m_message->flags.HasFlag(Message::Disabled)) {
        painter.fillRect(0, y, pixmap->width(), pixmap->height(), app->themes->messages.disabled);
    }

    // draw selection
    if (!selection.isEmpty()) {
        this->m_container.paintSelection(painter, messageIndex, selection, y);
    }

    // draw message seperation line
    if (app->settings->seperateMessages.getValue()) {
        painter.fillRect(0, y + this->m_container.getHeight() - 1, this->m_container.getWidth(), 1,
                         app->themes->splits.messageSeperator);
    }

    // draw last read message line
    if (isLastReadMessage) {
        QColor color = isWindowFocused ? app->themes->tabs.selected.backgrounds.regular.color()
                                       : app->themes->tabs.selected.backgrounds.unfocused.color();

        QBrush brush(color, Qt::VerPattern);

        painter.fillRect(0, y + this->m_container.getHeight() - 1, this->m_container.getWidth(), 1,
                         brush);
    }

    this->m_bufferValid = true;
}

void MessageLayout::updateBuffer(QPixmap *buffer, int /*messageIndex*/, Selection & /*selection*/)
{
    auto app = getApp();

    QPainter painter(buffer);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw background
    QColor backgroundColor;
    if (this->m_message->flags & Message::Highlighted) {
        backgroundColor = app->themes->messages.backgrounds.highlighted;
    } else if (app->settings->alternateMessageBackground.getValue() &&
               this->flags & MessageLayout::AlternateBackground) {
        backgroundColor = app->themes->messages.backgrounds.alternate;
    } else {
        backgroundColor = app->themes->messages.backgrounds.regular;
    }
    painter.fillRect(buffer->rect(), backgroundColor);

    // draw message
    this->m_container.paintElements(painter);

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
    this->m_bufferValid = false;
}

void MessageLayout::deleteBuffer()
{
    if (this->m_buffer != nullptr) {
        util::DebugCount::decrease("message drawing buffers");

        this->m_buffer = nullptr;
    }
}

void MessageLayout::deleteCache()
{
    this->deleteBuffer();

#ifdef XD
    this->m_container.clear();
#endif
}

// Elements
//    assert(QThread::currentThread() == QApplication::instance()->thread());

// returns nullptr if none was found

// fourtf: this should return a MessageLayoutItem
const MessageLayoutElement *MessageLayout::getElementAt(QPoint point)
{
    // go through all words and return the first one that contains the point.
    return this->m_container.getElementAt(point);
}

int MessageLayout::getLastCharacterIndex() const
{
    return this->m_container.getLastCharacterIndex();
}

int MessageLayout::getSelectionIndex(QPoint position)
{
    return this->m_container.getSelectionIndex(position);
}

void MessageLayout::addSelectionText(QString &str, int from, int to)
{
    this->m_container.addSelectionText(str, from, to);
}

}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
