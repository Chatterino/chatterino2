#include "messages/layouts/messagelayout.hpp"

#include "application.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/windowmanager.hpp"
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
    : message_(message)
    , buffer_(nullptr)
{
    util::DebugCount::increase("message layout");
}

MessageLayout::~MessageLayout()
{
    util::DebugCount::decrease("message layout");
}

Message *MessageLayout::getMessage()
{
    return this->message_.get();
}

// Height
int MessageLayout::getHeight() const
{
    return container_.getHeight();
}

// Layout
// return true if redraw is required
bool MessageLayout::layout(int width, float scale, MessageElement::Flags flags)
{
    //    BenchmarkGuard benchmark("MessageLayout::layout()");

    auto app = getApp();

    bool layoutRequired = false;

    // check if width changed
    bool widthChanged = width != this->currentLayoutWidth_;
    layoutRequired |= widthChanged;
    this->currentLayoutWidth_ = width;

    // check if layout state changed
    if (this->layoutState_ != app->windows->getGeneration()) {
        layoutRequired = true;
        this->flags |= RequiresBufferUpdate;
        this->layoutState_ = app->windows->getGeneration();
    }

    // check if work mask changed
    layoutRequired |= this->currentWordFlags_ != flags;
    this->currentWordFlags_ = flags;  // app->settings->getWordTypeMask();

    // check if layout was requested manually
    layoutRequired |= bool(this->flags & RequiresLayout);
    this->flags &= decltype(RequiresLayout)(~RequiresLayout);

    // check if dpi changed
    layoutRequired |= this->scale_ != scale;
    this->scale_ = scale;

    if (!layoutRequired) {
        return false;
    }

    int oldHeight = this->container_.getHeight();
    this->actuallyLayout(width, flags);
    if (widthChanged || this->container_.getHeight() != oldHeight) {
        this->deleteBuffer();
    }
    this->invalidateBuffer();

    return true;
}

void MessageLayout::actuallyLayout(int width, MessageElement::Flags _flags)
{
    auto messageFlags = this->message_->flags.value;

    if (this->flags & MessageLayout::Expanded ||
        (_flags & MessageElement::ModeratorTools &&
         !(this->message_->flags & Message::MessageFlags::Disabled))) {
        messageFlags = Message::MessageFlags(messageFlags & ~Message::MessageFlags::Collapsed);
    }

    this->container_.begin(width, this->scale_, messageFlags);

    for (const std::unique_ptr<MessageElement> &element : this->message_->getElements()) {
        element->addToContainer(this->container_, _flags);
    }

    if (this->height_ != this->container_.getHeight()) {
        this->deleteBuffer();
    }

    this->container_.end();
    this->height_ = this->container_.getHeight();

    // collapsed state
    this->flags &= ~Flags::Collapsed;
    if (this->container_.isCollapsed()) {
        this->flags |= Flags::Collapsed;
    }
}

// Painting
void MessageLayout::paint(QPainter &painter, int width, int y, int messageIndex,
                          Selection &selection, bool isLastReadMessage, bool isWindowFocused)
{
    auto app = getApp();
    QPixmap *pixmap = this->buffer_.get();

    // create new buffer if required
    if (!pixmap) {
#ifdef Q_OS_MACOS
        pixmap = new QPixmap(int(width * painter.device()->devicePixelRatioF()),
                             int(container_.getHeight() * painter.device()->devicePixelRatioF()));
        pixmap->setDevicePixelRatio(painter.device()->devicePixelRatioF());
#else
        pixmap = new QPixmap(width, std::max(16, this->container_.getHeight()));
#endif

        this->buffer_ = std::shared_ptr<QPixmap>(pixmap);
        this->bufferValid_ = false;
        util::DebugCount::increase("message drawing buffers");
    }

    if (!this->bufferValid_ || !selection.isEmpty()) {
        this->updateBuffer(pixmap, messageIndex, selection);
    }

    // draw on buffer
    painter.drawPixmap(0, y, *pixmap);
    //    painter.drawPixmap(0, y, this->container.width, this->container.getHeight(), *pixmap);

    // draw gif emotes
    this->container_.paintAnimatedElements(painter, y);

    // draw disabled
    if (this->message_->flags.HasFlag(Message::Disabled)) {
        painter.fillRect(0, y, pixmap->width(), pixmap->height(), app->themes->messages.disabled);
    }

    // draw selection
    if (!selection.isEmpty()) {
        this->container_.paintSelection(painter, messageIndex, selection, y);
    }

    // draw message seperation line
    if (app->settings->separateMessages.getValue()) {
        painter.fillRect(0, y, this->container_.getWidth(), 1,
                         app->themes->splits.messageSeperator);
    }

    // draw last read message line
    if (isLastReadMessage) {
        QColor color = isWindowFocused ? app->themes->tabs.selected.backgrounds.regular.color()
                                       : app->themes->tabs.selected.backgrounds.unfocused.color();

        QBrush brush(color, Qt::VerPattern);

        painter.fillRect(0, y + this->container_.getHeight() - 1, pixmap->width(), 1, brush);
    }

    this->bufferValid_ = true;
}

void MessageLayout::updateBuffer(QPixmap *buffer, int /*messageIndex*/, Selection & /*selection*/)
{
    auto app = getApp();

    QPainter painter(buffer);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw background
    QColor backgroundColor;
    if (this->message_->flags & Message::Highlighted) {
        backgroundColor = app->themes->messages.backgrounds.highlighted;
    } else if (this->message_->flags & Message::Subscription) {
        backgroundColor = app->themes->messages.backgrounds.subscription;
    } else if (app->settings->alternateMessageBackground.getValue() &&
               this->flags & MessageLayout::AlternateBackground) {
        backgroundColor = app->themes->messages.backgrounds.alternate;
    } else {
        backgroundColor = app->themes->messages.backgrounds.regular;
    }
    painter.fillRect(buffer->rect(), backgroundColor);

    // draw message
    this->container_.paintElements(painter);

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
    this->bufferValid_ = false;
}

void MessageLayout::deleteBuffer()
{
    if (this->buffer_ != nullptr) {
        util::DebugCount::decrease("message drawing buffers");

        this->buffer_ = nullptr;
    }
}

void MessageLayout::deleteCache()
{
    this->deleteBuffer();

#ifdef XD
    this->container_.clear();
#endif
}

// Elements
//    assert(QThread::currentThread() == QApplication::instance()->thread());

// returns nullptr if none was found

// fourtf: this should return a MessageLayoutItem
const MessageLayoutElement *MessageLayout::getElementAt(QPoint point)
{
    // go through all words and return the first one that contains the point.
    return this->container_.getElementAt(point);
}

int MessageLayout::getLastCharacterIndex() const
{
    return this->container_.getLastCharacterIndex();
}

int MessageLayout::getSelectionIndex(QPoint position)
{
    return this->container_.getSelectionIndex(position);
}

void MessageLayout::addSelectionText(QString &str, int from, int to)
{
    this->container_.addSelectionText(str, from, to);
}

}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
