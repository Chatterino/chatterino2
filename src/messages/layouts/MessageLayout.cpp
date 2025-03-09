#include "messages/layouts/MessageLayout.hpp"

#include "Application.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "messages/Selection.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QtGlobal>
#include <QThread>

namespace chatterino {

namespace {

    QColor blendColors(const QColor &base, const QColor &apply)
    {
        const qreal &alpha = apply.alphaF();
        QColor result;
        result.setRgbF(base.redF() * (1 - alpha) + apply.redF() * alpha,
                       base.greenF() * (1 - alpha) + apply.greenF() * alpha,
                       base.blueF() * (1 - alpha) + apply.blueF() * alpha);
        return result;
    }
}  // namespace

MessageLayout::MessageLayout(MessagePtr message)
    : message_(std::move(message))
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

const MessagePtr &MessageLayout::getMessagePtr() const
{
    return this->message_;
}

// Height
int MessageLayout::getHeight() const
{
    return this->container_.getHeight();
}

int MessageLayout::getWidth() const
{
    return this->container_.getWidth();
}

// Layout
// return true if redraw is required
bool MessageLayout::layout(const MessageLayoutContext &ctx,
                           bool shouldInvalidateBuffer)
{
    //    BenchmarkGuard benchmark("MessageLayout::layout()");

    bool layoutRequired = false;

    // check if width changed
    bool widthChanged = ctx.width != this->currentLayoutWidth_;
    layoutRequired |= widthChanged;
    this->currentLayoutWidth_ = ctx.width;

    // check if layout state changed
    const auto layoutGeneration = getApp()->getWindows()->getGeneration();
    if (this->layoutState_ != layoutGeneration)
    {
        layoutRequired = true;
        this->flags.set(MessageLayoutFlag::RequiresBufferUpdate);
        this->layoutState_ = layoutGeneration;
    }

    // check if work mask changed
    layoutRequired |= this->currentWordFlags_ != ctx.flags;
    this->currentWordFlags_ = ctx.flags;  // getSettings()->getWordTypeMask();

    // check if layout was requested manually
    layoutRequired |= this->flags.has(MessageLayoutFlag::RequiresLayout);
    this->flags.unset(MessageLayoutFlag::RequiresLayout);

    // check if dpi changed
    layoutRequired |= this->scale_ != ctx.scale;
    this->scale_ = ctx.scale;
    layoutRequired |= this->imageScale_ != ctx.imageScale;
    this->imageScale_ = ctx.imageScale;

    if (!layoutRequired)
    {
        if (shouldInvalidateBuffer)
        {
            this->invalidateBuffer();
            return true;
        }
        return false;
    }

    int oldHeight = this->container_.getHeight();
    this->actuallyLayout(ctx);
    if (widthChanged || this->container_.getHeight() != oldHeight)
    {
        this->deleteBuffer();
    }
    this->invalidateBuffer();

    return true;
}

void MessageLayout::actuallyLayout(const MessageLayoutContext &ctx)
{
#ifdef FOURTF
    this->layoutCount_++;
#endif

    auto messageFlags = this->message_->flags;

    if (this->flags.has(MessageLayoutFlag::Expanded) ||
        (ctx.flags.has(MessageElementFlag::ModeratorTools) &&
         !this->message_->flags.has(MessageFlag::Disabled)))
    {
        messageFlags.unset(MessageFlag::Collapsed);
    }

    bool hideModerated = getSettings()->hideModerated;
    bool hideModerationActions = getSettings()->hideModerationActions;
    bool hideBlockedTermAutomodMessages =
        getSettings()->showBlockedTermAutomodMessages.getEnum() ==
        ShowModerationState::Never;
    bool hideSimilar = getSettings()->hideSimilar;
    bool hideReplies = !ctx.flags.has(MessageElementFlag::RepliedMessage);

    this->container_.beginLayout(ctx.width, this->scale_, this->imageScale_,
                                 messageFlags);

    for (const auto &element : this->message_->elements)
    {
        if (hideModerated && this->message_->flags.has(MessageFlag::Disabled))
        {
            continue;
        }

        if (hideBlockedTermAutomodMessages &&
            this->message_->flags.has(MessageFlag::AutoModBlockedTerm))
        {
            // NOTE: This hides the message but it will make the message re-appear if moderation message hiding is no longer active, and the layout is re-laid-out.
            // This is only the case for the moderation messages that don't get filtered during creation.
            // We should decide which is the correct method & apply that everywhere
            continue;
        }

        if (this->message_->flags.has(MessageFlag::RestrictedMessage))
        {
            if (getApp()->getStreamerMode()->shouldHideRestrictedUsers())
            {
                // Message is being hidden because the source is a
                // restricted user
                continue;
            }
        }

        if (this->message_->flags.has(MessageFlag::ModerationAction))
        {
            if (hideModerationActions ||
                getApp()->getStreamerMode()->shouldHideModActions())
            {
                // Message is being hidden because we consider the message
                // a moderation action (something a streamer is unlikely to
                // want to share if they briefly show their chat on stream)
                continue;
            }
        }

        if (hideSimilar && this->message_->flags.has(MessageFlag::Similar))
        {
            continue;
        }

        if (hideReplies &&
            element->getFlags().has(MessageElementFlag::RepliedMessage))
        {
            continue;
        }

        element->addToContainer(this->container_, ctx);
    }

    if (this->height_ != this->container_.getHeight())
    {
        this->deleteBuffer();
    }

    this->container_.endLayout();
    this->height_ = this->container_.getHeight();

    // collapsed state
    this->flags.unset(MessageLayoutFlag::Collapsed);
    if (this->container_.isCollapsed())
    {
        this->flags.set(MessageLayoutFlag::Collapsed);
    }
}

// Painting
MessagePaintResult MessageLayout::paint(const MessagePaintContext &ctx)
{
    MessagePaintResult result;

    QPixmap *pixmap = this->ensureBuffer(ctx.painter, ctx.canvasWidth,
                                         ctx.messageColors.hasTransparency);

    if (!this->bufferValid_)
    {
        if (ctx.messageColors.hasTransparency)
        {
            pixmap->fill(Qt::transparent);
        }
        this->updateBuffer(pixmap, ctx);
    }

    // draw on buffer
    ctx.painter.drawPixmap(0, ctx.y, *pixmap);

    // draw gif emotes
    result.hasAnimatedElements =
        this->container_.paintAnimatedElements(ctx.painter, ctx.y);

    // draw disabled
    if (this->message_->flags.has(MessageFlag::Disabled))
    {
        ctx.painter.fillRect(0, ctx.y, pixmap->width(), pixmap->height(),
                             ctx.messageColors.disabled);
    }

    if (this->message_->flags.has(MessageFlag::RecentMessage))
    {
        ctx.painter.fillRect(0, ctx.y, pixmap->width(), pixmap->height(),
                             ctx.messageColors.disabled);
    }

    if (!ctx.isMentions &&
        (this->message_->flags.has(MessageFlag::RedeemedChannelPointReward) ||
         this->message_->flags.has(MessageFlag::RedeemedHighlight)) &&
        ctx.preferences.enableRedeemedHighlight)
    {
        ctx.painter.fillRect(
            0, ctx.y, int(this->scale_ * 4), pixmap->height(),
            *ColorProvider::instance().color(ColorType::RedeemedHighlight));
    }

    // draw selection
    if (!ctx.selection.isEmpty())
    {
        this->container_.paintSelection(ctx.painter, ctx.messageIndex,
                                        ctx.selection, ctx.y);
    }

    // draw message seperation line
    if (ctx.preferences.separateMessages)
    {
        ctx.painter.fillRect(0, ctx.y, this->container_.getWidth() + 64, 1,
                             ctx.messageColors.messageSeperator);
    }

    // draw last read message line
    if (ctx.isLastReadMessage)
    {
        QColor color;
        if (ctx.preferences.lastMessageColor.isValid())
        {
            color = ctx.preferences.lastMessageColor;
        }
        else
        {
            color = ctx.isWindowFocused
                        ? ctx.messageColors.focusedLastMessageLine
                        : ctx.messageColors.unfocusedLastMessageLine;
        }

        QBrush brush(color, ctx.preferences.lastMessagePattern);

        ctx.painter.fillRect(0, ctx.y + this->container_.getHeight() - 1,
                             pixmap->width(), 1, brush);
    }

    this->bufferValid_ = true;

    return result;
}

QPixmap *MessageLayout::ensureBuffer(QPainter &painter, int width, bool clear)
{
    if (this->buffer_ != nullptr)
    {
        return this->buffer_.get();
    }

    // Create new buffer
    this->buffer_ = std::make_unique<QPixmap>(
        int(width * painter.device()->devicePixelRatioF()),
        int(this->container_.getHeight() *
            painter.device()->devicePixelRatioF()));
    this->buffer_->setDevicePixelRatio(painter.device()->devicePixelRatioF());

    if (clear)
    {
        this->buffer_->fill(Qt::transparent);
    }

    this->bufferValid_ = false;
    DebugCount::increase("message drawing buffers");
    return this->buffer_.get();
}

void MessageLayout::updateBuffer(QPixmap *buffer,
                                 const MessagePaintContext &ctx)
{
    if (buffer->isNull())
    {
        return;
    }

    QPainter painter(buffer);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw background
    QColor backgroundColor = [&] {
        if (ctx.preferences.alternateMessages &&
            this->flags.has(MessageLayoutFlag::AlternateBackground))
        {
            return ctx.messageColors.alternateBg;
        }

        return ctx.messageColors.regularBg;
    }();

    if (this->message_->flags.has(MessageFlag::ElevatedMessage) &&
        ctx.preferences.enableElevatedMessageHighlight)
    {
        backgroundColor = blendColors(
            backgroundColor,
            *ctx.colorProvider.color(ColorType::ElevatedMessageHighlight));
    }

    else if (this->message_->flags.has(MessageFlag::FirstMessage) &&
             ctx.preferences.enableFirstMessageHighlight)
    {
        backgroundColor = blendColors(
            backgroundColor,
            *ctx.colorProvider.color(ColorType::FirstMessageHighlight));
    }
    else if ((this->message_->flags.has(MessageFlag::Highlighted) ||
              this->message_->flags.has(MessageFlag::HighlightedWhisper)) &&
             !this->flags.has(MessageLayoutFlag::IgnoreHighlights))
    {
        assert(this->message_->highlightColor);
        if (this->message_->highlightColor)
        {
            // Blend highlight color with usual background color
            backgroundColor =
                blendColors(backgroundColor, *this->message_->highlightColor);
        }
    }
    else if (this->message_->flags.has(MessageFlag::Subscription) &&
             ctx.preferences.enableSubHighlight)
    {
        // Blend highlight color with usual background color
        backgroundColor = blendColors(
            backgroundColor, *ctx.colorProvider.color(ColorType::Subscription));
    }
    else if ((this->message_->flags.has(MessageFlag::RedeemedHighlight) ||
              this->message_->flags.has(
                  MessageFlag::RedeemedChannelPointReward)) &&
             ctx.preferences.enableRedeemedHighlight)
    {
        // Blend highlight color with usual background color
        backgroundColor =
            blendColors(backgroundColor,
                        *ctx.colorProvider.color(ColorType::RedeemedHighlight));
    }
    else if (this->message_->flags.has(MessageFlag::AutoMod) ||
             this->message_->flags.has(MessageFlag::LowTrustUsers))
    {
        if (ctx.preferences.enableAutomodHighlight &&
            (this->message_->flags.has(MessageFlag::AutoModOffendingMessage) ||
             this->message_->flags.has(
                 MessageFlag::AutoModOffendingMessageHeader)))
        {
            backgroundColor = blendColors(
                backgroundColor,
                *ctx.colorProvider.color(ColorType::AutomodHighlight));
        }
        else
        {
            backgroundColor = QColor("#404040");
        }
    }
    else if (this->message_->flags.has(MessageFlag::Debug))
    {
        backgroundColor = QColor("#4A273D");
    }

    painter.fillRect(buffer->rect(), backgroundColor);

    // draw message
    this->container_.paintElements(painter, ctx);

#ifdef FOURTF
    // debug
    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(buffer->rect().x(), buffer->rect().y(),
                     buffer->rect().width() - 1, buffer->rect().height() - 1);

    QTextOption option;
    option.setAlignment(Qt::AlignRight | Qt::AlignTop);

    painter.drawText(QRectF(1, 1, this->container_.getWidth() - 3, 1000),
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
    this->container_.clear();
#endif
}

// Elements
//    assert(QThread::currentThread() == QApplication::instance()->thread());

// returns nullptr if none was found

// fourtf: this should return a MessageLayoutItem
const MessageLayoutElement *MessageLayout::getElementAt(QPoint point) const
{
    // go through all words and return the first one that contains the point.
    return this->container_.getElementAt(point);
}

std::pair<int, int> MessageLayout::getWordBounds(
    const MessageLayoutElement *hoveredElement, QPoint relativePos) const
{
    // An element with wordId != -1 can be multiline, so we need to check all
    // elements in the container
    if (hoveredElement->getWordId() != -1)
    {
        return this->container_.getWordBounds(hoveredElement);
    }

    const auto wordStart = this->getSelectionIndex(relativePos) -
                           hoveredElement->getMouseOverIndex(relativePos);
    const auto selectionLength = hoveredElement->getSelectionIndexCount();
    const auto length = hoveredElement->hasTrailingSpace() ? selectionLength - 1
                                                           : selectionLength;

    return {wordStart, wordStart + length};
}

size_t MessageLayout::getLastCharacterIndex() const
{
    return this->container_.getLastCharacterIndex();
}

size_t MessageLayout::getFirstMessageCharacterIndex() const
{
    return this->container_.getFirstMessageCharacterIndex();
}

size_t MessageLayout::getSelectionIndex(QPoint position) const
{
    return this->container_.getSelectionIndex(position);
}

void MessageLayout::addSelectionText(QString &str, uint32_t from, uint32_t to,
                                     CopyMode copymode)
{
    this->container_.addSelectionText(str, from, to, copymode);
}

bool MessageLayout::isReplyable() const
{
    if (this->message_->loginName.isEmpty())
    {
        return false;
    }

    if (this->message_->flags.hasAny(
            {MessageFlag::System, MessageFlag::Subscription,
             MessageFlag::Timeout, MessageFlag::Whisper,
             MessageFlag::ModerationAction}))
    {
        return false;
    }

    return true;
}

}  // namespace chatterino
