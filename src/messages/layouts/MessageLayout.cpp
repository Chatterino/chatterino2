#include "messages/layouts/MessageLayout.hpp"

#include "Application.hpp"
#include "debug/Benchmark.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "messages/Selection.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"
#include "util/StreamerMode.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QtGlobal>
#include <QThread>

#define MARGIN_LEFT (int)(8 * this->scale)
#define MARGIN_RIGHT (int)(8 * this->scale)
#define MARGIN_TOP (int)(4 * this->scale)
#define MARGIN_BOTTOM (int)(4 * this->scale)
#define COMPACT_EMOTES_OFFSET 6

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

    int oldHeight = this->container_.getHeight();
    this->actuallyLayout(width, flags);
    if (widthChanged || this->container_.getHeight() != oldHeight)
    {
        this->deleteBuffer();
    }
    this->invalidateBuffer();

    return true;
}

void MessageLayout::actuallyLayout(int width, MessageElementFlags flags)
{
#ifdef FOURTF
    this->layoutCount_++;
#endif

    auto messageFlags = this->message_->flags;

    if (this->flags.has(MessageLayoutFlag::Expanded) ||
        (flags.has(MessageElementFlag::ModeratorTools) &&
         !this->message_->flags.has(MessageFlag::Disabled)))
    {
        messageFlags.unset(MessageFlag::Collapsed);
    }

    bool hideModerated = getSettings()->hideModerated;
    bool hideModerationActions = getSettings()->hideModerationActions;
    bool hideSimilar = getSettings()->hideSimilar;
    bool hideReplies = !flags.has(MessageElementFlag::RepliedMessage);

    this->container_.begin(width, this->scale_, messageFlags);

    for (const auto &element : this->message_->elements)
    {
        if (hideModerated && this->message_->flags.has(MessageFlag::Disabled))
        {
            continue;
        }

        if (this->message_->flags.has(MessageFlag::Timeout) ||
            this->message_->flags.has(MessageFlag::Untimeout))
        {
            // This condition has been set up to execute isInStreamerMode() as the last thing
            // as it could end up being expensive.
            if (hideModerationActions ||
                (getSettings()->streamerModeHideModActions &&
                 isInStreamerMode()))
            {
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

        element->addToContainer(this->container_, flags);
    }

    if (this->height_ != this->container_.getHeight())
    {
        this->deleteBuffer();
    }

    this->container_.end();
    this->height_ = this->container_.getHeight();

    // collapsed state
    this->flags.unset(MessageLayoutFlag::Collapsed);
    if (this->container_.isCollapsed())
    {
        this->flags.set(MessageLayoutFlag::Collapsed);
    }
}

// Painting
void MessageLayout::paint(QPainter &painter, int width, int y, int messageIndex,
                          Selection &selection, bool isLastReadMessage,
                          bool isWindowFocused, bool isMentions)
{
    auto app = getApp();
    QPixmap *pixmap = this->ensureBuffer(painter, width);

    if (!this->bufferValid_ || !selection.isEmpty())
    {
        this->updateBuffer(pixmap, messageIndex, selection);
    }

    // draw on buffer
    painter.drawPixmap(0, y, *pixmap);
    //    painter.drawPixmap(0, y, this->container.width,
    //    this->container.getHeight(), *pixmap);

    // draw gif emotes
    this->container_.paintAnimatedElements(painter, y);

    // draw disabled
    if (this->message_->flags.has(MessageFlag::Disabled))
    {
        painter.fillRect(0, y, pixmap->width(), pixmap->height(),
                         app->themes->messages.disabled);
        //        painter.fillRect(0, y, pixmap->width(), pixmap->height(),
        //                         QBrush(QColor(64, 64, 64, 64)));
    }

    if (this->message_->flags.has(MessageFlag::RecentMessage) &&
        getSettings()->grayOutRecents)
    {
        painter.fillRect(0, y, pixmap->width(), pixmap->height(),
                         app->themes->messages.disabled);
    }

    if (!isMentions &&
        (this->message_->flags.has(MessageFlag::RedeemedChannelPointReward) ||
         this->message_->flags.has(MessageFlag::RedeemedHighlight)) &&
        getSettings()->enableRedeemedHighlight.getValue())
    {
        painter.fillRect(
            0, y, this->scale_ * 4, pixmap->height(),
            *ColorProvider::instance().color(ColorType::RedeemedHighlight));
    }

    // draw selection
    if (!selection.isEmpty())
    {
        this->container_.paintSelection(painter, messageIndex, selection, y);
    }

    // draw message seperation line
    if (getSettings()->separateMessages.getValue())
    {
        painter.fillRect(0, y, this->container_.getWidth() + 64, 1,
                         app->themes->splits.messageSeperator);
    }

    // draw last read message line
    if (isLastReadMessage)
    {
        QColor color;
        if (getSettings()->lastMessageColor != QStringLiteral(""))
        {
            color = QColor(getSettings()->lastMessageColor.getValue());
        }
        else
        {
            color = isWindowFocused
                        ? app->themes->tabs.selected.backgrounds.regular
                        : app->themes->tabs.selected.backgrounds.unfocused;
        }

        QBrush brush(color, static_cast<Qt::BrushStyle>(
                                getSettings()->lastMessagePattern.getValue()));

        painter.fillRect(0, y + this->container_.getHeight() - 1,
                         pixmap->width(), 1, brush);
    }

    this->bufferValid_ = true;
}

QPixmap *MessageLayout::ensureBuffer(QPainter &painter, int width)
{
    if (this->buffer_ != nullptr)
    {
        return this->buffer_.get();
    }

    // Create new buffer
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    this->buffer_ = std::make_unique<QPixmap>(
        int(width * painter.device()->devicePixelRatioF()),
        int(this->container_.getHeight() *
            painter.device()->devicePixelRatioF()));
    this->buffer_->setDevicePixelRatio(painter.device()->devicePixelRatioF());
#else
    this->buffer_ = std::make_unique<QPixmap>(
        width, std::max(16, this->container_.getHeight()));
#endif

    this->bufferValid_ = false;
    DebugCount::increase("message drawing buffers");
    return this->buffer_.get();
}

void MessageLayout::updateBuffer(QPixmap *buffer, int /*messageIndex*/,
                                 Selection & /*selection*/)
{
    if (buffer->isNull())
        return;

    auto app = getApp();
    auto settings = getSettings();

    QPainter painter(buffer);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw background
    QColor backgroundColor = [this, &app] {
        if (getSettings()->alternateMessages.getValue() &&
            this->flags.has(MessageLayoutFlag::AlternateBackground))
        {
            return app->themes->messages.backgrounds.alternate;
        }
        else
        {
            return app->themes->messages.backgrounds.regular;
        }
    }();

    if (this->message_->flags.has(MessageFlag::ElevatedMessage) &&
        getSettings()->enableElevatedMessageHighlight.getValue())
    {
        backgroundColor = blendColors(backgroundColor,
                                      *ColorProvider::instance().color(
                                          ColorType::ElevatedMessageHighlight));
    }

    else if (this->message_->flags.has(MessageFlag::FirstMessage) &&
             getSettings()->enableFirstMessageHighlight.getValue())
    {
        backgroundColor = blendColors(
            backgroundColor,
            *ColorProvider::instance().color(ColorType::FirstMessageHighlight));
    }
    else if ((this->message_->flags.has(MessageFlag::Highlighted) ||
              this->message_->flags.has(MessageFlag::HighlightedWhisper)) &&
             !this->flags.has(MessageLayoutFlag::IgnoreHighlights))
    {
        // Blend highlight color with usual background color
        backgroundColor =
            blendColors(backgroundColor, *this->message_->highlightColor);
    }
    else if (this->message_->flags.has(MessageFlag::Subscription) &&
             getSettings()->enableSubHighlight)
    {
        // Blend highlight color with usual background color
        backgroundColor = blendColors(
            backgroundColor,
            *ColorProvider::instance().color(ColorType::Subscription));
    }
    else if ((this->message_->flags.has(MessageFlag::RedeemedHighlight) ||
              this->message_->flags.has(
                  MessageFlag::RedeemedChannelPointReward)) &&
             settings->enableRedeemedHighlight.getValue())
    {
        // Blend highlight color with usual background color
        backgroundColor = blendColors(
            backgroundColor,
            *ColorProvider::instance().color(ColorType::RedeemedHighlight));
    }
    else if (this->message_->flags.has(MessageFlag::AutoMod))
    {
        backgroundColor = QColor("#404040");
    }
    else if (this->message_->flags.has(MessageFlag::Debug))
    {
        backgroundColor = QColor("#4A273D");
    }
    else if (this->message_->flags.has(MessageFlag::WebchatDetected) &&
             getSettings()->normalNonceDetection)
    {
        backgroundColor =
            blendColors(backgroundColor, QColor(getSettings()->webchatColor));
    }

    painter.fillRect(buffer->rect(), backgroundColor);

    // draw message
    this->container_.paintElements(painter);

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
const MessageLayoutElement *MessageLayout::getElementAt(QPoint point)
{
    // go through all words and return the first one that contains the point.
    return this->container_.getElementAt(point);
}

int MessageLayout::getLastCharacterIndex() const
{
    return this->container_.getLastCharacterIndex();
}

int MessageLayout::getFirstMessageCharacterIndex() const
{
    return this->container_.getFirstMessageCharacterIndex();
}

int MessageLayout::getSelectionIndex(QPoint position)
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
             MessageFlag::Timeout, MessageFlag::Whisper}))
    {
        return false;
    }

    return true;
}

}  // namespace chatterino
