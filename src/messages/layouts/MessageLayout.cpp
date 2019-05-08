#include "messages/layouts/MessageLayout.hpp"

#include "Application.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"

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

MessageLayout::MessageLayout(MessagePtr message)
    : message_(message)
    , container_(std::make_shared<MessageLayoutContainer>())
{
    DebugCount::increase("message layout");
}

MessageLayout::~MessageLayout()
{
    DebugCount::decrease("message layout");
}

const Message *MessageLayout::getMessage()
{
    return this->message_.get();
}

// Height
int MessageLayout::getHeight() const
{
    return container_->getHeight();
}

// Layout
// return true if redraw is required
bool MessageLayout::layout(int width, float scale, MessageElementFlags flags)
{
    //    BenchmarkGuard benchmark("MessageLayout::layout()");

    auto app = getApp();

    bool layoutRequired = false;

    // check if width changed
    bool widthChanged = width != this->currentLayoutWidth_;
    layoutRequired |= widthChanged;
    this->currentLayoutWidth_ = width;

    // check if layout state changed
    if (this->layoutState_ != app->windows->getGeneration())
    {
        layoutRequired = true;
        this->flags.set(MessageLayoutFlag::RequiresBufferUpdate);
        this->layoutState_ = app->windows->getGeneration();
    }

    // check if work mask changed
    layoutRequired |= this->currentWordFlags_ != flags;
    this->currentWordFlags_ = flags;  // getSettings()->getWordTypeMask();

    // check if layout was requested manually
    layoutRequired |= this->flags.has(MessageLayoutFlag::RequiresLayout);
    this->flags.unset(MessageLayoutFlag::RequiresLayout);

    // check if dpi changed
    layoutRequired |= this->scale_ != scale;
    this->scale_ = scale;

    if (!layoutRequired)
    {
        return false;
    }

    int oldHeight = this->container_->getHeight();
    this->actuallyLayout(width, flags);
    if (widthChanged || this->container_->getHeight() != oldHeight)
    {
        this->deleteBuffer();
    }
    this->invalidateBuffer();

    return true;
}

void MessageLayout::actuallyLayout(int width, MessageElementFlags flags)
{
    this->layoutCount_++;

    auto messageFlags = this->message_->flags;

    if (this->flags.has(MessageLayoutFlag::Expanded) ||
        (flags.has(MessageElementFlag::ModeratorTools) &&
         !this->message_->flags.has(MessageFlag::Disabled)))  //
    {
        messageFlags.unset(MessageFlag::Collapsed);
    }

    this->container_->begin(width, this->scale_, messageFlags);

    for (const auto &element : this->message_->elements)
    {
        if (getSettings()->hideModerated &&
            this->message_->flags.has(MessageFlag::Disabled))
        {
            continue;
        }
        element->addToContainer(*this->container_, flags);
    }

    if (this->height_ != this->container_->getHeight())
    {
        this->deleteBuffer();
    }

    this->container_->end();
    this->height_ = this->container_->getHeight();

    // collapsed state
    this->flags.unset(MessageLayoutFlag::Collapsed);
    if (this->container_->isCollapsed())
    {
        this->flags.set(MessageLayoutFlag::Collapsed);
    }
}

// Painting
void MessageLayout::paint(QPainter &painter, int width, int y, int messageIndex,
                          Selection &selection, bool isLastReadMessage,
                          bool isWindowFocused)
{
    auto app = getApp();
    QPixmap *pixmap = this->buffer_.get();

    // create new buffer if required
    if (!pixmap)
    {
#ifdef Q_OS_MACOS
        pixmap = new QPixmap(int(width * painter.device()->devicePixelRatioF()),
                             int(container_->getHeight() *
                                 painter.device()->devicePixelRatioF()));
        pixmap->setDevicePixelRatio(painter.device()->devicePixelRatioF());
#else
        pixmap =
            new QPixmap(width, std::max(16, this->container_->getHeight()));
#endif

        this->buffer_ = std::shared_ptr<QPixmap>(pixmap);
        this->bufferValid_ = false;
        DebugCount::increase("message drawing buffers");
    }

    if (!this->bufferValid_ || !selection.isEmpty())
    {
        this->updateBuffer(pixmap, messageIndex, selection);
    }

    // draw on buffer
    painter.drawPixmap(0, y, *pixmap);
    //    painter.drawPixmap(0, y, this->container.width,
    //    this->container.getHeight(), *pixmap);

    // draw gif emotes
    this->container_->paintAnimatedElements(painter, y);

    // draw disabled
    if (this->message_->flags.has(MessageFlag::Disabled))
    {
        painter.fillRect(0, y, pixmap->width(), pixmap->height(),
                         app->themes->messages.disabled);
        //        painter.fillRect(0, y, pixmap->width(), pixmap->height(),
        //                         QBrush(QColor(64, 64, 64, 64)));
    }

    if (this->message_->flags.has(MessageFlag::RecentMessage))
    {
        painter.fillRect(0, y, pixmap->width(), pixmap->height(),
                         app->themes->messages.disabled);
    }

    // draw selection
    if (!selection.isEmpty())
    {
        this->container_->paintSelection(painter, messageIndex, selection, y);
    }

    // draw message seperation line
    if (getSettings()->separateMessages.getValue())
    {
        painter.fillRect(0, y, this->container_->getWidth() + 64, 1,
                         app->themes->splits.messageSeperator);
    }

    // draw last read message line
    if (isLastReadMessage)
    {
        QColor color;
        if (getSettings()->lastMessageColor != "")
        {
            color = QColor(getSettings()->lastMessageColor.getValue());
        }
        else
        {
            color =
                isWindowFocused
                    ? app->themes->tabs.selected.backgrounds.regular.color()
                    : app->themes->tabs.selected.backgrounds.unfocused.color();
        }

        QBrush brush(color, static_cast<Qt::BrushStyle>(
                                getSettings()->lastMessagePattern.getValue()));

        painter.fillRect(0, y + this->container_->getHeight() - 1,
                         pixmap->width(), 1, brush);
    }

    this->bufferValid_ = true;
}

void MessageLayout::updateBuffer(QPixmap *buffer, int /*messageIndex*/,
                                 Selection & /*selection*/)
{
    auto app = getApp();

    QPainter painter(buffer);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw background
    QColor backgroundColor = app->themes->messages.backgrounds.regular;
    if (this->message_->flags.has(MessageFlag::Highlighted) &&
        !this->flags.has(MessageLayoutFlag::IgnoreHighlights))
    {
        backgroundColor = app->themes->messages.backgrounds.highlighted;
    }
    else if (this->message_->flags.has(MessageFlag::Subscription))
    {
        backgroundColor = app->themes->messages.backgrounds.subscription;
    }
    else if (getSettings()->alternateMessages.getValue() &&
             this->flags.has(MessageLayoutFlag::AlternateBackground))
    {
        backgroundColor = app->themes->messages.backgrounds.alternate;
    }
    else if (this->message_->flags.has(MessageFlag::AutoMod))
    {
        backgroundColor = QColor("#404040");
    }

    painter.fillRect(buffer->rect(), backgroundColor);

    // draw message
    this->container_->paintElements(painter);

#ifdef FOURTF
    // debug
    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(buffer->rect().x(), buffer->rect().y(),
                     buffer->rect().width() - 1, buffer->rect().height() - 1);

    QTextOption option;
    option.setAlignment(Qt::AlignRight | Qt::AlignTop);

    painter.drawText(QRectF(1, 1, this->container_->getWidth() - 3, 1000),
                     QString::number(this->layoutCount_) + ", " +
                         QString::number(++this->bufferUpdatedCount_),
                     option);
#endif
}

void MessageLayout::invalidateBuffer()
{
    this->bufferValid_ = false;
}

void MessageLayout::deleteBuffer()
{
    if (this->buffer_ != nullptr)
    {
        DebugCount::decrease("message drawing buffers");

        this->buffer_ = nullptr;
    }
}

void MessageLayout::deleteCache()
{
    this->deleteBuffer();

#ifdef XD
    this->container_->clear();
#endif
}

// Elements
//    assert(QThread::currentThread() == QApplication::instance()->thread());

// returns nullptr if none was found

// fourtf: this should return a MessageLayoutItem
const MessageLayoutElement *MessageLayout::getElementAt(QPoint point)
{
    // go through all words and return the first one that contains the point.
    return this->container_->getElementAt(point);
}

int MessageLayout::getLastCharacterIndex() const
{
    return this->container_->getLastCharacterIndex();
}

int MessageLayout::getFirstMessageCharacterIndex() const
{
    return this->container_->getFirstMessageCharacterIndex();
}

int MessageLayout::getSelectionIndex(QPoint position)
{
    return this->container_->getSelectionIndex(position);
}

void MessageLayout::addSelectionText(QString &str, int from, int to,
                                     CopyMode copymode)
{
    this->container_->addSelectionText(str, from, to, copymode);
}

}  // namespace chatterino
