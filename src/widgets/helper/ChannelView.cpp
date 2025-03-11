#include "widgets/helper/ChannelView.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/filters/FilterSet.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "messages/MessageThread.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "providers/links/LinkInfo.hpp"
#include "providers/links/LinkResolver.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clipboard.hpp"
#include "util/DistanceBetweenPoints.hpp"
#include "util/Helpers.hpp"
#include "util/IncognitoBrowser.hpp"
#include "util/QMagicEnum.hpp"
#include "util/Twitch.hpp"
#include "widgets/dialogs/ReplyThreadPopup.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"
#include "widgets/helper/SearchPopup.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitInput.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/Window.hpp"

#include <magic_enum/magic_enum_flags.hpp>
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QEasingCurve>
#include <QGestureEvent>
#include <QGraphicsBlurEffect>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPainter>
#include <QScreen>
#include <QVariantAnimation>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>

namespace {

constexpr size_t TOOLTIP_EMOTE_ENTRIES_LIMIT = 7;

using namespace chatterino;

constexpr int SCROLLBAR_PADDING = 8;

void addEmoteContextMenuItems(QMenu *menu, const Emote &emote,
                              MessageElementFlags creatorFlags)
{
    auto *openAction = menu->addAction("&Open");
    auto *openMenu = new QMenu(menu);
    openAction->setMenu(openMenu);

    auto *copyAction = menu->addAction("&Copy");
    auto *copyMenu = new QMenu(menu);
    copyAction->setMenu(copyMenu);

    // Scale of the smallest image
    std::optional<qreal> baseScale;
    // Add copy and open links for images
    auto addImageLink = [&](const ImagePtr &image) {
        if (!image->isEmpty())
        {
            if (!baseScale)
            {
                baseScale = image->scale();
            }

            auto factor =
                QString::number(static_cast<int>(*baseScale / image->scale()));
            copyMenu->addAction("&" + factor + "x link", [url = image->url()] {
                crossPlatformCopy(url.string);
            });
            openMenu->addAction("&" + factor + "x link", [url = image->url()] {
                QDesktopServices::openUrl(QUrl(url.string));
            });
        }
    };

    addImageLink(emote.images.getImage1());
    addImageLink(emote.images.getImage2());
    addImageLink(emote.images.getImage3());

    // Copy and open emote page link
    auto addPageLink = [&](const QString &name) {
        copyMenu->addSeparator();
        openMenu->addSeparator();

        copyMenu->addAction("Copy " + name + " &emote link",
                            [url = emote.homePage] {
                                crossPlatformCopy(url.string);
                            });
        openMenu->addAction("Open " + name + " &emote link",
                            [url = emote.homePage] {
                                QDesktopServices::openUrl(QUrl(url.string));
                            });
    };

    if (creatorFlags.has(MessageElementFlag::BttvEmote))
    {
        addPageLink("BTTV");
    }
    else if (creatorFlags.has(MessageElementFlag::FfzEmote))
    {
        addPageLink("FFZ");
    }
    else if (creatorFlags.has(MessageElementFlag::SevenTVEmote))
    {
        addPageLink("7TV");
    }
}

void addImageContextMenuItems(QMenu *menu,
                              const MessageLayoutElement *hoveredElement)
{
    if (hoveredElement == nullptr)
    {
        return;
    }

    const auto &creator = hoveredElement->getCreator();
    auto creatorFlags = creator.getFlags();

    // Badge actions
    if (creatorFlags.hasAny({MessageElementFlag::Badges}))
    {
        if (const auto *badgeElement =
                dynamic_cast<const BadgeElement *>(&creator))
        {
            addEmoteContextMenuItems(menu, *badgeElement->getEmote(),
                                     creatorFlags);
        }
    }

    // Emote actions
    if (creatorFlags.hasAny(
            {MessageElementFlag::EmoteImages, MessageElementFlag::EmojiImage}))
    {
        if (const auto *emoteElement =
                dynamic_cast<const EmoteElement *>(&creator))
        {
            addEmoteContextMenuItems(menu, *emoteElement->getEmote(),
                                     creatorFlags);
        }
        else if (const auto *layeredElement =
                     dynamic_cast<const LayeredEmoteElement *>(&creator))
        {
            // Give each emote its own submenu
            for (auto &emote : layeredElement->getUniqueEmotes())
            {
                auto *emoteAction = menu->addAction(emote.ptr->name.string);
                auto *emoteMenu = new QMenu(menu);
                emoteAction->setMenu(emoteMenu);
                addEmoteContextMenuItems(emoteMenu, *emote.ptr, emote.flags);
            }
        }
    }

    // add seperator
    if (!menu->actions().empty())
    {
        menu->addSeparator();
    }
}

void addLinkContextMenuItems(QMenu *menu,
                             const MessageLayoutElement *hoveredElement)
{
    if (hoveredElement == nullptr)
    {
        return;
    }

    const auto &link = hoveredElement->getLink();

    if (link.type != Link::Url)
    {
        return;
    }

    // Link copy
    QString url = link.value;

    // open link
    menu->addAction("&Open link", [url] {
        QDesktopServices::openUrl(QUrl(url));
    });
    // open link default
    if (supportsIncognitoLinks())
    {
        menu->addAction("Open link &incognito", [url] {
            openLinkIncognito(url);
        });
    }
    menu->addAction("&Copy link", [url] {
        crossPlatformCopy(url);
    });

    menu->addSeparator();
}

void addHiddenContextMenuItems(QMenu *menu,
                               const MessageLayoutElement * /*hoveredElement*/,
                               const MessageLayoutPtr &layout,
                               QMouseEvent *event)
{
    if (!layout)
    {
        return;
    }

    if (event->modifiers() != Qt::ShiftModifier)
    {
        // NOTE: We currently require the modifier to be ONLY shift - we might want to check if shift is among the modifiers instead
        return;
    }

    if (!layout->getMessage()->id.isEmpty())
    {
        menu->addAction("Copy message &ID",
                        [messageID = layout->getMessage()->id] {
                            crossPlatformCopy(messageID);
                        });
    }

    auto message = layout->getMessagePtr();

    if (message)
    {
        menu->addAction("Copy message &JSON", [message] {
            auto jsonString = QJsonDocument{message->toJson()}.toJson(
                QJsonDocument::Indented);
            crossPlatformCopy(QString::fromUtf8(jsonString));
        });
    }
}

// Current function: https://www.desmos.com/calculator/vdyamchjwh
qreal highlightEasingFunction(qreal progress)
{
    if (progress <= 0.1)
    {
        return 1.0 - pow(10.0 * progress, 3.0);
    }
    return 1.0 + pow((20.0 / 9.0) * (0.5 * progress - 0.5), 3.0);
}

}  // namespace

namespace chatterino {

ChannelView::ChannelView(QWidget *parent, Context context, size_t messagesLimit)
    : ChannelView(InternalCtor{}, parent, nullptr, context, messagesLimit)
{
}

ChannelView::ChannelView(QWidget *parent, Split *split, Context context,
                         size_t messagesLimit)
    : ChannelView(InternalCtor{}, parent, split, context, messagesLimit)
{
    assert(parent != nullptr && split != nullptr &&
           "This constructor should only be used with non-null values (see "
           "documentation)");
}

ChannelView::ChannelView(InternalCtor /*tag*/, QWidget *parent, Split *split,
                         Context context, size_t messagesLimit)
    : BaseWidget(parent)
    , channel_(Channel::getEmpty())
    , split_(split)
    , scrollBar_(new Scrollbar(messagesLimit, this))
    , highlightAnimation_(this)
    , context_(context)
    , messages_(messagesLimit)
    , tooltipWidget_(new TooltipWidget(this))
{
    this->setMouseTracking(true);

    this->initializeLayout();
    this->initializeScrollbar();
    this->initializeSignals();

    this->cursors_.neutral = QCursor(getResources().scrolling.neutralScroll);
    this->cursors_.up = QCursor(getResources().scrolling.upScroll);
    this->cursors_.down = QCursor(getResources().scrolling.downScroll);

    this->pauseTimer_.setSingleShot(true);
    QObject::connect(&this->pauseTimer_, &QTimer::timeout, this, [this] {
        // remove elements that are finite
        std::erase_if(this->pauses_, [](const auto &p) {
            return p.second.has_value();
        });

        this->updatePauses();
    });

    // This shortcut is not used in splits, it's used in views that
    // don't have a SplitInput like the SearchPopup or EmotePopup.
    // See SplitInput::installKeyPressedEvent for the copy event
    // from views with a SplitInput.
    auto *shortcut = new QShortcut(QKeySequence::StandardKey::Copy, this);
    QObject::connect(shortcut, &QShortcut::activated, [this] {
        this->copySelectedText();
    });

    this->clickTimer_.setSingleShot(true);
    this->clickTimer_.setInterval(500);

    this->scrollTimer_.setInterval(20);
    QObject::connect(&this->scrollTimer_, &QTimer::timeout, this, [this] {
        this->scrollUpdateRequested();
    });

    this->grabGesture(Qt::PanGesture);

    // TODO: Figure out if we need this, and if so, why
    // StrongFocus means we can focus this event through clicking it
    // and tabbing to it from another widget. I don't currently know
    // of any place where you can, or where it would make sense,
    // to tab to a ChannelVieChannelView
    this->setFocusPolicy(Qt::FocusPolicy::ClickFocus);

    this->setupHighlightAnimationColors();
    this->highlightAnimation_.setDuration(1500);
    auto curve = QEasingCurve();
    curve.setCustomType(highlightEasingFunction);
    this->highlightAnimation_.setEasingCurve(curve);
    QObject::connect(&this->highlightAnimation_,
                     &QVariantAnimation::valueChanged, this, [this] {
                         this->queueUpdate();
                     });

    this->messageColors_.applyTheme(getTheme(), this->isOverlay_,
                                    getSettings()->overlayBackgroundOpacity);
    this->messagePreferences_.connectSettings(getSettings(),
                                              this->signalHolder_);
}

void ChannelView::initializeLayout()
{
    this->goToBottom_ = new EffectLabel(this, 0);
    this->goToBottom_->setStyleSheet(
        "background-color: rgba(0,0,0,0.66); color: #FFF;");
    this->goToBottom_->getLabel().setText("More messages below");
    this->goToBottom_->setVisible(false);

    QObject::connect(
        this->goToBottom_, &EffectLabel::leftClicked, this, [this] {
            QTimer::singleShot(180, this, [this] {
                this->scrollBar_->scrollToBottom(
                    getSettings()->enableSmoothScrollingNewMessages.getValue());
            });
        });
}

void ChannelView::initializeScrollbar()
{
    // We can safely ignore the scroll bar's signal connection since the scroll bar will
    // always be destroyed before the ChannelView
    std::ignore = this->scrollBar_->getCurrentValueChanged().connect([this] {
        if (this->isVisible())
        {
            this->performLayout(true);
            this->queueUpdate();
        }
        else
        {
            this->layoutQueued_ = true;
        }
    });
}

void ChannelView::initializeSignals()
{
    this->signalHolder_.managedConnect(getApp()->getWindows()->wordFlagsChanged,
                                       [this] {
                                           this->queueLayout();
                                           this->update();
                                       });

    getSettings()->showLastMessageIndicator.connect(
        [this](auto, auto) {
            this->update();
        },
        this->signalHolder_);

    this->signalHolder_.managedConnect(
        getApp()->getWindows()->gifRepaintRequested, [&] {
            if (!this->animationArea_.isEmpty())
            {
                this->queueUpdate(this->animationArea_);
            }
        });

    this->signalHolder_.managedConnect(
        getApp()->getWindows()->layoutRequested, [&](Channel *channel) {
            if (this->isVisible() &&
                (channel == nullptr ||
                 this->underlyingChannel_.get() == channel))
            {
                this->queueLayout();
            }
        });

    this->signalHolder_.managedConnect(
        getApp()->getWindows()->invalidateBuffersRequested,
        [this](Channel *channel) {
            if (this->isVisible() &&
                (channel == nullptr ||
                 this->underlyingChannel_.get() == channel))
            {
                this->invalidateBuffers();
            }
        });

    this->signalHolder_.managedConnect(getApp()->getFonts()->fontChanged,
                                       [this] {
                                           this->queueLayout();
                                       });
}

Scrollbar *ChannelView::scrollbar()
{
    return this->scrollBar_;
}

bool ChannelView::pausable() const
{
    return pausable_;
}

void ChannelView::setPausable(bool value)
{
    this->pausable_ = value;
}

bool ChannelView::paused() const
{
    /// No elements in the map -> not paused
    return this->pausable() && !this->pauses_.empty();
}

void ChannelView::pause(PauseReason reason, std::optional<uint> msecs)
{
    bool wasUnpaused = !this->paused();

    if (msecs)
    {
        /// Msecs has a value
        auto timePoint = SteadyClock::now() + std::chrono::milliseconds(*msecs);
        auto it = this->pauses_.find(reason);

        if (it == this->pauses_.end())
        {
            /// No value found so we insert a new one.
            this->pauses_[reason] = timePoint;
        }
        else
        {
            /// If the new time point is newer then we override.
            auto &previousTimePoint = it->second;
            if (previousTimePoint.has_value() &&
                previousTimePoint.value() < timePoint)
            {
                previousTimePoint = timePoint;
            }
        }
    }
    else
    {
        /// Msecs is none -> pause is infinite.
        /// We just override the value.
        this->pauses_[reason] = std::nullopt;
    }

    this->updatePauses();

    if (wasUnpaused)
    {
        this->update();
    }
}

void ChannelView::unpause(PauseReason reason)
{
    if (this->pauses_.erase(reason) > 0)
    {
        this->updatePauses();
    }
}

void ChannelView::updatePauses()
{
    using namespace std::chrono;

    if (this->pauses_.empty())
    {
        this->unpaused();

        /// No pauses so we can stop the timer
        this->pauseEnd_ = std::nullopt;
        this->pauseTimer_.stop();

        this->scrollBar_->offsetMaximum(this->pauseScrollMaximumOffset_);
        this->scrollBar_->offsetMinimum(this->pauseScrollMinimumOffset_);
        this->pauseScrollMinimumOffset_ = 0;
        this->pauseScrollMaximumOffset_ = 0;

        this->queueLayout();
        // make sure we re-render
        this->update();
    }
    else if (std::any_of(this->pauses_.begin(), this->pauses_.end(),
                         [](auto &&value) {
                             return !value.second;
                         }))
    {
        /// Some of the pauses are infinite
        this->pauseEnd_ = std::nullopt;
        this->pauseTimer_.stop();
    }
    else
    {
        /// Get the maximum pause
        auto pauseEnd =
            std::max_element(this->pauses_.begin(), this->pauses_.end(),
                             [](auto &&a, auto &&b) {
                                 return a.second > b.second;
                             })
                ->second.value();

        if (pauseEnd != this->pauseEnd_)
        {
            /// Start the timer
            this->pauseEnd_ = pauseEnd;
            auto duration =
                duration_cast<milliseconds>(pauseEnd - SteadyClock::now());
            this->pauseTimer_.start(std::max(duration, 0ms));
        }
    }
}

void ChannelView::unpaused()
{
    /// Move selection
    this->selection_.shiftMessageIndex(this->pauseSelectionOffset_);
    this->doubleClickSelection_.shiftMessageIndex(this->pauseSelectionOffset_);

    this->pauseSelectionOffset_ = 0;
}

void ChannelView::themeChangedEvent()
{
    BaseWidget::themeChangedEvent();

    this->setupHighlightAnimationColors();
    this->queueLayout();
    this->messageColors_.applyTheme(getTheme(), this->isOverlay_,
                                    getSettings()->overlayBackgroundOpacity);
}

void ChannelView::updateColorTheme()
{
    this->themeChangedEvent();
}

void ChannelView::setIsOverlay(bool isOverlay)
{
    this->isOverlay_ = isOverlay;
    this->themeChangedEvent();
}

void ChannelView::setupHighlightAnimationColors()
{
    this->highlightAnimation_.setStartValue(
        this->theme->messages.highlightAnimationStart);
    this->highlightAnimation_.setEndValue(
        this->theme->messages.highlightAnimationEnd);
}

void ChannelView::scaleChangedEvent(float scale)
{
    BaseWidget::scaleChangedEvent(scale);

    if (this->goToBottom_)
    {
        auto factor = this->scale();
#ifdef Q_OS_MACOS
        factor = scale * 80.F /
                 std::max<float>(
                     0.01, this->logicalDpiX() * this->devicePixelRatioF());
#endif
        this->goToBottom_->getLabel().setFont(
            getApp()->getFonts()->getFont(FontStyle::UiMedium, factor));
    }
}

void ChannelView::queueUpdate()
{
    this->update();
}

void ChannelView::queueUpdate(const QRect &area)
{
    this->update(area);
}

void ChannelView::invalidateBuffers()
{
    this->bufferInvalidationQueued_ = true;
    this->queueLayout();
}

void ChannelView::queueLayout()
{
    if (this->isVisible())
    {
        this->performLayout();
    }
    else
    {
        this->layoutQueued_ = true;
    }
}

void ChannelView::showEvent(QShowEvent * /*event*/)
{
    if (this->layoutQueued_)
    {
        this->performLayout(false, true);
    }
}

void ChannelView::performLayout(bool causedByScrollbar, bool causedByShow)
{
    // BenchmarkGuard benchmark("layout");

    this->layoutQueued_ = false;

    /// Get messages and check if there are at least 1
    const auto &messages = this->getMessagesSnapshot();

    this->showingLatestMessages_ =
        this->scrollBar_->isAtBottom() ||
        (!this->scrollBar_->isVisible() && !causedByScrollbar);

    /// Layout visible messages
    this->layoutVisibleMessages(messages);

    /// Update scrollbar
    this->updateScrollbar(messages, causedByScrollbar, causedByShow);

    this->goToBottom_->setVisible(this->enableScrollingToBottom_ &&
                                  this->scrollBar_->isVisible() &&
                                  !this->scrollBar_->isAtBottom());
}

void ChannelView::layoutVisibleMessages(
    const LimitedQueueSnapshot<MessageLayoutPtr> &messages)
{
    const auto start = size_t(this->scrollBar_->getRelativeCurrentValue());
    const auto layoutWidth = this->getLayoutWidth();
    const auto flags = this->getFlags();
    auto redrawRequired = false;

    if (messages.size() > start)
    {
        auto y = int(-(messages[start]->getHeight() *
                       (fmod(this->scrollBar_->getRelativeCurrentValue(), 1))));

        for (auto i = start; i < messages.size() && y <= this->height(); i++)
        {
            const auto &message = messages[i];

            redrawRequired |= message->layout(
                {
                    .messageColors = this->messageColors_,
                    .flags = flags,
                    .width = layoutWidth,
                    .scale = this->scale(),
                    .imageScale = this->scale() *
                                  static_cast<float>(this->devicePixelRatio()),
                },
                this->bufferInvalidationQueued_);

            y += message->getHeight();
        }
    }
    this->bufferInvalidationQueued_ = false;

    if (redrawRequired)
    {
        this->queueUpdate();
    }
}

void ChannelView::updateScrollbar(
    const LimitedQueueSnapshot<MessageLayoutPtr> &messages,
    bool causedByScrollbar, bool causedByShow)
{
    if (messages.size() == 0)
    {
        this->scrollBar_->setVisible(false);
        return;
    }

    /// Layout the messages at the bottom
    auto h = this->height() - 8;
    auto flags = this->getFlags();
    auto layoutWidth = this->getLayoutWidth();
    auto showScrollbar = false;

    // convert i to int since it checks >= 0
    for (auto i = int(messages.size()) - 1; i >= 0; i--)
    {
        auto *message = messages[i].get();

        message->layout(
            {
                .messageColors = this->messageColors_,
                .flags = flags,
                .width = layoutWidth,
                .scale = this->scale(),
                .imageScale = this->scale() *
                              static_cast<float>(this->devicePixelRatio()),
            },
            false);

        h -= message->getHeight();

        if (h < 0)  // break condition
        {
            this->scrollBar_->setPageSize(
                (messages.size() - i) +
                qreal(h) / std::max<int>(1, message->getHeight()));

            showScrollbar = true;
            break;
        }
    }

    /// Update scrollbar values
    this->scrollBar_->setVisible(showScrollbar);

    if (!showScrollbar && !causedByScrollbar)
    {
        this->scrollBar_->scrollToTop();
    }
    this->showScrollBar_ = showScrollbar;

    // If we were showing the latest messages and the scrollbar now wants to be
    // rendered, scroll to bottom
    if (this->enableScrollingToBottom_ && this->showingLatestMessages_ &&
        showScrollbar && !causedByScrollbar)
    {
        this->scrollBar_->scrollToBottom(
            !causedByShow &&
            getSettings()->enableSmoothScrollingNewMessages.getValue());
    }
}

void ChannelView::clearMessages()
{
    // Clear all stored messages in this chat widget
    this->messages_.clear();
    this->scrollBar_->clearHighlights();
    this->scrollBar_->resetBounds();
    this->scrollBar_->setMaximum(0);
    this->scrollBar_->setMinimum(0);
    this->queueLayout();
    this->update();

    this->lastMessageHasAlternateBackground_ = false;
    this->lastMessageHasAlternateBackgroundReverse_ = true;
}

Scrollbar &ChannelView::getScrollBar()
{
    return *this->scrollBar_;
}

QString ChannelView::getSelectedText()
{
    QString result = "";

    LimitedQueueSnapshot<MessageLayoutPtr> &messagesSnapshot =
        this->getMessagesSnapshot();

    Selection selection = this->selection_;

    if (selection.isEmpty())
    {
        return result;
    }

    const auto numMessages = messagesSnapshot.size();
    const auto indexStart = selection.selectionMin.messageIndex;
    const auto indexEnd = selection.selectionMax.messageIndex;

    if (indexEnd >= numMessages || indexStart >= numMessages)
    {
        // One of our messages is out of bounds
        return result;
    }

    for (auto msg = indexStart; msg <= indexEnd; msg++)
    {
        MessageLayoutPtr layout = messagesSnapshot[msg];
        auto from = msg == selection.selectionMin.messageIndex
                        ? selection.selectionMin.charIndex
                        : 0;
        auto to = msg == selection.selectionMax.messageIndex
                      ? selection.selectionMax.charIndex
                      : layout->getLastCharacterIndex() + 1;

        layout->addSelectionText(result, from, to);

        if (msg != indexEnd)
        {
            result += '\n';
        }
    }

    return result;
}

bool ChannelView::hasSelection()
{
    return !this->selection_.isEmpty();
}

void ChannelView::clearSelection()
{
    this->selection_ = Selection();
    queueLayout();
}

void ChannelView::copySelectedText()
{
    crossPlatformCopy(this->getSelectedText());
}

void ChannelView::setEnableScrollingToBottom(bool value)
{
    this->enableScrollingToBottom_ = value;
}

bool ChannelView::getEnableScrollingToBottom() const
{
    return this->enableScrollingToBottom_;
}

void ChannelView::setOverrideFlags(std::optional<MessageElementFlags> value)
{
    this->overrideFlags_ = value;
}

const std::optional<MessageElementFlags> &ChannelView::getOverrideFlags() const
{
    return this->overrideFlags_;
}

LimitedQueueSnapshot<MessageLayoutPtr> &ChannelView::getMessagesSnapshot()
{
    this->snapshotGuard_.guard();
    if (!this->paused() /*|| this->scrollBar_->isVisible()*/)
    {
        this->snapshot_ = this->messages_.getSnapshot();
    }

    return this->snapshot_;
}

ChannelPtr ChannelView::channel() const
{
    assert(this->channel_ != nullptr);

    return this->channel_;
}

ChannelPtr ChannelView::underlyingChannel() const
{
    return this->underlyingChannel_;
}

bool ChannelView::showScrollbarHighlights() const
{
    return this->channel_->getType() != Channel::Type::TwitchMentions;
}

void ChannelView::setChannel(const ChannelPtr &underlyingChannel)
{
    /// Clear connections from the last channel
    this->channelConnections_.clear();

    this->clearMessages();
    this->scrollBar_->clearHighlights();

    /// make copy of channel and expose
    this->channel_ = std::make_unique<Channel>(underlyingChannel->getName(),
                                               underlyingChannel->getType());

    //
    // Proxy channel connections
    // Use a proxy channel to keep filtered messages past the time they are removed from their origin channel
    //

    this->channelConnections_.managedConnect(
        underlyingChannel->messageAppended,
        [this](MessagePtr &message,
               std::optional<MessageFlags> overridingFlags) {
            if (this->shouldIncludeMessage(message))
            {
                if (this->channel_->lastDate_ != QDate::currentDate())
                {
                    // Day change message
                    this->channel_->lastDate_ = QDate::currentDate();
                    auto msg = makeSystemMessage(
                        QLocale().toString(QDate::currentDate(),
                                           QLocale::LongFormat),
                        QTime(0, 0));
                    msg->flags.set(MessageFlag::DoNotLog);
                    this->channel_->addMessage(msg, MessageContext::Original);
                }
                this->channel_->addMessage(message, MessageContext::Repost,
                                           overridingFlags);
            }
        });

    this->channelConnections_.managedConnect(
        underlyingChannel->messagesAddedAtStart,
        [this](std::vector<MessagePtr> &messages) {
            std::vector<MessagePtr> filtered;
            std::copy_if(messages.begin(), messages.end(),
                         std::back_inserter(filtered), [this](const auto &msg) {
                             return this->shouldIncludeMessage(msg);
                         });

            if (!filtered.empty())
            {
                this->channel_->addMessagesAtStart(filtered);
            }
        });

    this->channelConnections_.managedConnect(
        underlyingChannel->messageReplaced,
        [this](auto index, const auto &prev, const auto &replacement) {
            if (this->shouldIncludeMessage(replacement))
            {
                this->channel_->replaceMessage(index, prev, replacement);
            }
        });

    this->channelConnections_.managedConnect(
        underlyingChannel->filledInMessages, [this](const auto &messages) {
            std::vector<MessagePtr> filtered;
            filtered.reserve(messages.size());
            std::copy_if(messages.begin(), messages.end(),
                         std::back_inserter(filtered), [this](const auto &msg) {
                             return this->shouldIncludeMessage(msg);
                         });
            this->channel_->fillInMissingMessages(filtered);
        });

    this->channelConnections_.managedConnect(underlyingChannel->messagesCleared,
                                             [this]() {
                                                 this->clearMessages();
                                             });

    // Copy over messages from the backing channel to the filtered one
    // and the ui.
    auto snapshot = underlyingChannel->getMessageSnapshot();

    size_t nMessagesAdded = 0;
    for (const auto &msg : snapshot)
    {
        if (!this->shouldIncludeMessage(msg))
        {
            continue;
        }

        auto messageLayout = std::make_shared<MessageLayout>(msg);

        if (this->lastMessageHasAlternateBackground_)
        {
            messageLayout->flags.set(MessageLayoutFlag::AlternateBackground);
        }
        this->lastMessageHasAlternateBackground_ =
            !this->lastMessageHasAlternateBackground_;

        if (underlyingChannel->shouldIgnoreHighlights())
        {
            messageLayout->flags.set(MessageLayoutFlag::IgnoreHighlights);
        }

        this->messages_.pushBack(messageLayout);

        this->channel_->addMessage(msg, MessageContext::Repost);

        nMessagesAdded++;
        if (this->showScrollbarHighlights())
        {
            this->scrollBar_->addHighlight(msg->getScrollBarHighlight());
        }
    }

    this->scrollBar_->setMaximum(
        static_cast<qreal>(std::min(nMessagesAdded, this->messages_.limit())));

    //
    // Standard channel connections
    //

    // on new message
    this->channelConnections_.managedConnect(
        this->channel_->messageAppended,
        [this](MessagePtr &message,
               std::optional<MessageFlags> overridingFlags) {
            this->messageAppended(message, overridingFlags);
        });

    this->channelConnections_.managedConnect(
        this->channel_->messagesAddedAtStart,
        [this](std::vector<MessagePtr> &messages) {
            this->messageAddedAtStart(messages);
        });

    // on message replaced
    this->channelConnections_.managedConnect(
        this->channel_->messageReplaced,
        [this](size_t index, const MessagePtr &prev,
               const MessagePtr &replacement) {
            this->messageReplaced(index, prev, replacement);
        });

    // on messages filled in
    this->channelConnections_.managedConnect(this->channel_->filledInMessages,
                                             [this](const auto &) {
                                                 this->messagesUpdated();
                                             });

    this->underlyingChannel_ = underlyingChannel;

    this->updateID();

    this->queueLayout();
    if (!this->isVisible() && !this->scrollBar_->isVisible())
    {
        // If we're not visible and the scrollbar is not (yet) visible,
        // we need to make sure that it's at the bottom when this view is laid
        // out later.
        this->scrollBar_->scrollToBottom();
    }
    this->queueUpdate();

    // Notifications
    auto *twitchChannel =
        dynamic_cast<TwitchChannel *>(underlyingChannel.get());
    if (twitchChannel != nullptr)
    {
        this->channelConnections_.managedConnect(
            twitchChannel->streamStatusChanged, [this]() {
                this->liveStatusChanged.invoke();
            });
    }
}

void ChannelView::setFilters(const QList<QUuid> &ids)
{
    this->channelFilters_ = std::make_shared<FilterSet>(ids);

    this->updateID();
}

QList<QUuid> ChannelView::getFilterIds() const
{
    if (!this->channelFilters_)
    {
        return {};
    }

    return this->channelFilters_->filterIds();
}

FilterSetPtr ChannelView::getFilterSet() const
{
    return this->channelFilters_;
}

bool ChannelView::shouldIncludeMessage(const MessagePtr &m) const
{
    if (this->channelFilters_)
    {
        if (getSettings()->excludeUserMessagesFromFilter &&
            getApp()->getAccounts()->twitch.getCurrent()->getUserName().compare(
                m->loginName, Qt::CaseInsensitive) == 0)
        {
            return true;
        }

        return this->channelFilters_->filter(m, this->underlyingChannel_);
    }

    return true;
}

ChannelPtr ChannelView::sourceChannel() const
{
    return this->sourceChannel_;
}

void ChannelView::setSourceChannel(ChannelPtr sourceChannel)
{
    this->sourceChannel_ = std::move(sourceChannel);
}

bool ChannelView::hasSourceChannel() const
{
    return this->sourceChannel_ != nullptr;
}

void ChannelView::messageAppended(MessagePtr &message,
                                  std::optional<MessageFlags> overridingFlags)
{
    auto *messageFlags = &message->flags;
    if (overridingFlags)
    {
        messageFlags = &*overridingFlags;
    }

    auto messageRef = std::make_shared<MessageLayout>(message);

    if (this->lastMessageHasAlternateBackground_)
    {
        messageRef->flags.set(MessageLayoutFlag::AlternateBackground);
    }
    if (this->channel_->shouldIgnoreHighlights())
    {
        messageRef->flags.set(MessageLayoutFlag::IgnoreHighlights);
    }
    this->lastMessageHasAlternateBackground_ =
        !this->lastMessageHasAlternateBackground_;

    if (this->paused())
    {
        this->pauseScrollMaximumOffset_++;
    }
    else
    {
        this->scrollBar_->offsetMaximum(1);
    }

    if (this->messages_.pushBack(messageRef))
    {
        if (this->paused())
        {
            this->pauseScrollMinimumOffset_++;
            this->pauseSelectionOffset_++;
        }
        else
        {
            this->scrollBar_->offsetMinimum(1);
            if (this->showingLatestMessages_ && !this->isVisible())
            {
                this->scrollBar_->scrollToBottom(false);
            }
            this->selection_.shiftMessageIndex(1);
            this->doubleClickSelection_.shiftMessageIndex(1);
        }
    }

    if (!messageFlags->has(MessageFlag::DoNotTriggerNotification))
    {
        if ((messageFlags->has(MessageFlag::Highlighted) &&
             messageFlags->has(MessageFlag::ShowInMentions) &&
             !messageFlags->has(MessageFlag::Subscription) &&
             (getSettings()->highlightMentions ||
              this->channel_->getType() != Channel::Type::TwitchMentions)) ||
            (this->channel_->getType() == Channel::Type::TwitchAutomod &&
             getSettings()->enableAutomodHighlight))
        {
            this->tabHighlightRequested.invoke(HighlightState::Highlighted);
        }
        else
        {
            this->tabHighlightRequested.invoke(HighlightState::NewMessage);
        }
    }

    if (this->showScrollbarHighlights())
    {
        this->scrollBar_->addHighlight(message->getScrollBarHighlight());
    }

    this->queueLayout();
}

void ChannelView::messageAddedAtStart(std::vector<MessagePtr> &messages)
{
    std::vector<MessageLayoutPtr> messageRefs;
    messageRefs.resize(messages.size());

    /// Create message layouts
    for (size_t i = 0; i < messages.size(); i++)
    {
        auto message = messages.at(i);
        auto layout = std::make_shared<MessageLayout>(message);

        // alternate color
        if (!this->lastMessageHasAlternateBackgroundReverse_)
        {
            layout->flags.set(MessageLayoutFlag::AlternateBackground);
        }
        this->lastMessageHasAlternateBackgroundReverse_ =
            !this->lastMessageHasAlternateBackgroundReverse_;

        messageRefs.at(i) = std::move(layout);
    }

    /// Add the messages at the start
    auto addedMessages = this->messages_.pushFront(messageRefs);
    if (!addedMessages.empty())
    {
        if (this->scrollBar_->isAtBottom())
        {
            this->scrollBar_->scrollToBottom();
        }
        else
        {
            this->scrollBar_->offset(qreal(addedMessages.size()));
        }
        this->scrollBar_->offsetMaximum(qreal(addedMessages.size()));
    }

    if (this->showScrollbarHighlights())
    {
        std::vector<ScrollbarHighlight> highlights;
        highlights.reserve(messages.size());
        for (const auto &message : messages)
        {
            highlights.push_back(message->getScrollBarHighlight());
        }

        this->scrollBar_->addHighlightsAtStart(highlights);
    }

    this->queueLayout();
}

void ChannelView::messageReplaced(size_t hint, const MessagePtr &prev,
                                  const MessagePtr &replacement)
{
    auto optItem = this->messages_.find(hint, [&](const auto &it) {
        return it->getMessagePtr() == prev;
    });
    if (!optItem)
    {
        return;
    }
    const auto &[index, oldItem] = *optItem;

    auto newItem = std::make_shared<MessageLayout>(replacement);

    if (oldItem->flags.has(MessageLayoutFlag::AlternateBackground))
    {
        newItem->flags.set(MessageLayoutFlag::AlternateBackground);
    }

    this->scrollBar_->replaceHighlight(index,
                                       replacement->getScrollBarHighlight());

    this->messages_.replaceItem(index, newItem);
    this->queueLayout();
}

void ChannelView::messagesUpdated()
{
    auto snapshot = this->channel_->getMessageSnapshot();

    this->messages_.clear();
    this->scrollBar_->clearHighlights();
    this->scrollBar_->resetBounds();
    this->scrollBar_->setMaximum(qreal(snapshot.size()));
    this->scrollBar_->setMinimum(0);
    this->lastMessageHasAlternateBackground_ = false;
    this->lastMessageHasAlternateBackgroundReverse_ = true;

    for (const auto &msg : snapshot)
    {
        auto messageLayout = std::make_shared<MessageLayout>(msg);

        if (this->lastMessageHasAlternateBackground_)
        {
            messageLayout->flags.set(MessageLayoutFlag::AlternateBackground);
        }
        this->lastMessageHasAlternateBackground_ =
            !this->lastMessageHasAlternateBackground_;

        if (this->channel_->shouldIgnoreHighlights())
        {
            messageLayout->flags.set(MessageLayoutFlag::IgnoreHighlights);
        }

        this->messages_.pushBack(messageLayout);
        if (this->showScrollbarHighlights())
        {
            this->scrollBar_->addHighlight(msg->getScrollBarHighlight());
        }
    }

    this->queueLayout();
}

void ChannelView::updateLastReadMessage()
{
    if (auto lastMessage = this->messages_.last())
    {
        this->lastReadMessage_ = *lastMessage;
    }

    this->update();
}

void ChannelView::resizeEvent(QResizeEvent * /*event*/)
{
    this->scrollBar_->setGeometry(this->width() - this->scrollBar_->width(), 0,
                                  this->scrollBar_->width(), this->height());

    this->goToBottom_->setGeometry(0, this->height() - int(this->scale() * 26),
                                   this->width(), int(this->scale() * 26));

    this->scrollBar_->raise();

    this->queueLayout();

    this->update();
}

void ChannelView::setSelection(const Selection &newSelection)
{
    if (this->selection_ != newSelection)
    {
        this->selection_ = newSelection;
        this->selectionChanged.invoke();
        this->update();
    }
}

void ChannelView::setSelection(const SelectionItem &start,
                               const SelectionItem &end)
{
    this->setSelection({start, end});
}

MessageElementFlags ChannelView::getFlags() const
{
    auto *app = getApp();

    if (this->overrideFlags_)
    {
        return *this->overrideFlags_;
    }

    MessageElementFlags flags = app->getWindows()->getWordFlags();

    auto *split = dynamic_cast<Split *>(this->parentWidget());

    if (split == nullptr)
    {
        auto *searchPopup = dynamic_cast<SearchPopup *>(this->parentWidget());
        if (searchPopup != nullptr)
        {
            split = dynamic_cast<Split *>(searchPopup->parentWidget());
        }
    }

    if (split != nullptr)
    {
        if (split->getModerationMode())
        {
            flags.set(MessageElementFlag::ModeratorTools);
        }
        if (this->underlyingChannel_ ==
                getApp()->getTwitch()->getMentionsChannel() ||
            this->underlyingChannel_ ==
                getApp()->getTwitch()->getLiveChannel() ||
            this->underlyingChannel_ ==
                getApp()->getTwitch()->getAutomodChannel())
        {
            flags.set(MessageElementFlag::ChannelName);
            flags.unset(MessageElementFlag::ChannelPointReward);
        }
    }

    if (this->sourceChannel_ == getApp()->getTwitch()->getMentionsChannel() ||
        this->sourceChannel_ == getApp()->getTwitch()->getAutomodChannel())
    {
        flags.set(MessageElementFlag::ChannelName);
    }

    if (this->context_ == Context::ReplyThread ||
        getSettings()->hideReplyContext)
    {
        // Don't show inline replies within the ReplyThreadPopup
        // or if they're hidden
        flags.unset(MessageElementFlag::RepliedMessage);
    }

    if (!this->canReplyToMessages())
    {
        flags.unset(MessageElementFlag::ReplyButton);
    }

    return flags;
}

bool ChannelView::scrollToMessage(const MessagePtr &message)
{
    if (!this->mayContainMessage(message))
    {
        return false;
    }

    auto &messagesSnapshot = this->getMessagesSnapshot();
    if (messagesSnapshot.size() == 0)
    {
        return false;
    }

    // TODO: Figure out if we can somehow binary-search here.
    //       Currently, a message only sometimes stores a QDateTime,
    //       but always a QTime (inaccurate on midnight).
    //
    // We're searching from the bottom since it's more likely for a user
    // wanting to go to a message that recently scrolled out of view.
    size_t messageIdx = messagesSnapshot.size() - 1;
    for (; messageIdx < SIZE_MAX; messageIdx--)
    {
        if (messagesSnapshot[messageIdx]->getMessagePtr() == message)
        {
            break;
        }
    }

    if (messageIdx == SIZE_MAX)
    {
        return false;
    }

    this->scrollToMessageLayout(messagesSnapshot[messageIdx].get(), messageIdx);
    if (this->split_)
    {
        getApp()->getWindows()->select(this->split_);
    }
    return true;
}

bool ChannelView::scrollToMessageId(const QString &messageId)
{
    auto &messagesSnapshot = this->getMessagesSnapshot();
    if (messagesSnapshot.size() == 0)
    {
        return false;
    }

    // We're searching from the bottom since it's more likely for a user
    // wanting to go to a message that recently scrolled out of view.
    size_t messageIdx = messagesSnapshot.size() - 1;
    for (; messageIdx < SIZE_MAX; messageIdx--)
    {
        if (messagesSnapshot[messageIdx]->getMessagePtr()->id == messageId)
        {
            break;
        }
    }

    if (messageIdx == SIZE_MAX)
    {
        return false;
    }

    this->scrollToMessageLayout(messagesSnapshot[messageIdx].get(), messageIdx);
    if (this->split_)
    {
        getApp()->getWindows()->select(this->split_);
    }
    return true;
}

void ChannelView::scrollToMessageLayout(MessageLayout *layout,
                                        size_t messageIdx)
{
    this->highlightedMessage_ = layout;
    this->highlightAnimation_.setCurrentTime(0);
    this->highlightAnimation_.start(QAbstractAnimation::KeepWhenStopped);

    if (this->showScrollBar_)
    {
        this->getScrollBar().setDesiredValue(this->scrollBar_->getMinimum() +
                                             qreal(messageIdx));
    }
}

void ChannelView::paintEvent(QPaintEvent *event)
{
    //    BenchmarkGuard benchmark("paint");

    QPainter painter(this);

    painter.fillRect(rect(), this->messageColors_.channelBackground);

    // draw messages
    this->drawMessages(painter, event->rect());

    // draw paused sign
    if (this->paused())
    {
        auto a = this->scale() * 20;
        auto brush = QBrush(QColor(127, 127, 127, 255));
        painter.fillRect(QRectF(5, a / 4, a / 4, a), brush);
        painter.fillRect(QRectF(15, a / 4, a / 4, a), brush);
    }
}

// if overlays is false then it draws the message, if true then it draws things
// such as the grey overlay when a message is disabled
void ChannelView::drawMessages(QPainter &painter, const QRect &area)
{
    auto &messagesSnapshot = this->getMessagesSnapshot();

    const auto start = size_t(this->scrollBar_->getRelativeCurrentValue());

    if (start >= messagesSnapshot.size())
    {
        return;
    }

    MessageLayout *end = nullptr;

    MessagePaintContext ctx = {
        .painter = painter,
        .selection = this->selection_,
        .colorProvider = ColorProvider::instance(),
        .messageColors = this->messageColors_,
        .preferences = this->messagePreferences_,

        .canvasWidth = this->width(),
        .isWindowFocused = this->window() == QApplication::activeWindow(),
        .isMentions = this->underlyingChannel_ ==
                      getApp()->getTwitch()->getMentionsChannel(),

        .y = int(-(messagesSnapshot[start]->getHeight() *
                   (fmod(this->scrollBar_->getRelativeCurrentValue(), 1)))),
        .messageIndex = start,
        .isLastReadMessage = false,

    };
    bool showLastMessageIndicator = getSettings()->showLastMessageIndicator;

    QRect animationArea;
    auto areaContainsY = [&area](auto y) {
        return y >= area.y() && y < area.y() + area.height();
    };

    for (; ctx.messageIndex < messagesSnapshot.size(); ++ctx.messageIndex)
    {
        MessageLayout *layout = messagesSnapshot[ctx.messageIndex].get();

        if (showLastMessageIndicator)
        {
            ctx.isLastReadMessage = this->lastReadMessage_.get() == layout;
        }
        else
        {
            ctx.isLastReadMessage = false;
        }

        if (areaContainsY(ctx.y) ||
            areaContainsY(ctx.y + layout->getHeight()) ||
            (ctx.y < area.y() && layout->getHeight() > area.height()))
        {
            auto paintResult = layout->paint(ctx);
            if (paintResult.hasAnimatedElements)
            {
                if (animationArea.isNull())
                {
                    animationArea = QRect{0, ctx.y, layout->getWidth(),
                                          layout->getHeight()};
                }
                else
                {
                    animationArea.setBottom(ctx.y + layout->getHeight());
                    animationArea.setWidth(
                        std::max(layout->getWidth(), animationArea.width()));
                }
            }

            if (this->highlightedMessage_ == layout)
            {
                painter.fillRect(
                    0, ctx.y, layout->getWidth(), layout->getHeight(),
                    this->highlightAnimation_.currentValue().value<QColor>());
                if (this->highlightAnimation_.state() ==
                    QVariantAnimation::Stopped)
                {
                    this->highlightedMessage_ = nullptr;
                }
            }
        }

        ctx.y += layout->getHeight();

        end = layout;
        if (ctx.y > this->height())
        {
            break;
        }
    }

    // Only update on a full repaint as some messages with animated elements
    // might get left out in partial repaints.
    // This happens for example when hovering over the go-to-bottom button.
    if (this->height() <= area.height())
    {
        this->animationArea_ = animationArea;
    }
#ifdef FOURTF
    else
    {
        // shows the updated area on partial repaints
        painter.setPen(Qt::red);
        painter.drawRect(area.x(), area.y(), area.width() - 1,
                         area.height() - 1);
    }
#endif

    if (end == nullptr)
    {
        return;
    }

    // remove messages that are on screen
    // the messages that are left at the end get their buffers reset
    for (size_t i = start; i < messagesSnapshot.size(); ++i)
    {
        auto it = this->messagesOnScreen_.find(messagesSnapshot[i]);
        if (it != this->messagesOnScreen_.end())
        {
            this->messagesOnScreen_.erase(it);
        }
    }

    // delete the message buffers that aren't on screen
    for (const std::shared_ptr<MessageLayout> &item : this->messagesOnScreen_)
    {
        item->deleteBuffer();
    }

    this->messagesOnScreen_.clear();

    // add all messages on screen to the map
    for (size_t i = start; i < messagesSnapshot.size(); ++i)
    {
        const std::shared_ptr<MessageLayout> &layout = messagesSnapshot[i];

        this->messagesOnScreen_.insert(layout);

        if (layout.get() == end)
        {
            break;
        }
    }
}

void ChannelView::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() == 0)
    {
        // Ignore any scrolls where no vertical scrolling has taken place
        return;
    }

    if (event->modifiers().testFlag(Qt::ControlModifier))
    {
        // Ignore any scrolls where ctrl is held down - it is used for zoom
        event->ignore();
        return;
    }

    if (this->scrollBar_->isVisible())
    {
        float mouseMultiplier = getSettings()->mouseScrollMultiplier;

        // This ensures snapshot won't be indexed out of bounds when scrolling really fast
        qreal desired = std::max<qreal>(0, this->scrollBar_->getDesiredValue());
        qreal delta = event->angleDelta().y() * qreal(1.5) * mouseMultiplier;

        auto &snapshot = this->getMessagesSnapshot();
        int snapshotLength = int(snapshot.size());
        int i = std::min<int>(int(desired - this->scrollBar_->getMinimum()),
                              snapshotLength - 1);

        if (delta > 0)
        {
            qreal scrollFactor = fmod(desired, 1);
            qreal currentScrollLeft = std::max<qreal>(
                0.01, int(scrollFactor * snapshot[i]->getHeight()));

            for (; i >= 0; i--)
            {
                if (delta < currentScrollLeft)
                {
                    desired -= scrollFactor * (delta / currentScrollLeft);
                    break;
                }
                else
                {
                    delta -= currentScrollLeft;
                    desired -= scrollFactor;
                }

                if (i == 0)
                {
                    desired = 0;
                }
                else
                {
                    snapshot[i - 1]->layout(
                        {
                            .messageColors = this->messageColors_,
                            .flags = this->getFlags(),
                            .width = this->getLayoutWidth(),
                            .scale = this->scale(),
                            .imageScale =
                                this->scale() *
                                static_cast<float>(this->devicePixelRatio()),
                        },
                        false);
                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i - 1]->getHeight();
                }
            }
        }
        else
        {
            delta = -delta;
            qreal scrollFactor = 1 - fmod(desired, 1);
            qreal currentScrollLeft = std::max<qreal>(
                0.01, int(scrollFactor * snapshot[i]->getHeight()));

            for (; i < snapshotLength; i++)
            {
                if (delta < currentScrollLeft)
                {
                    desired +=
                        scrollFactor * (qreal(delta) / currentScrollLeft);
                    break;
                }
                else
                {
                    delta -= currentScrollLeft;
                    desired += scrollFactor;
                }

                if (i == snapshotLength - 1)
                {
                    desired = snapshot.size();
                }
                else
                {
                    snapshot[i + 1]->layout(
                        {
                            .messageColors = this->messageColors_,
                            .flags = this->getFlags(),
                            .width = this->getLayoutWidth(),
                            .scale = this->scale(),
                            .imageScale =
                                this->scale() *
                                static_cast<float>(this->devicePixelRatio()),
                        },
                        false);

                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i + 1]->getHeight();
                }
            }
        }

        this->scrollBar_->setDesiredValue(desired, true);
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ChannelView::enterEvent(QEnterEvent * /*event*/)
#else
void ChannelView::enterEvent(QEvent * /*event*/)
#endif
{
}

void ChannelView::leaveEvent(QEvent * /*event*/)
{
    this->tooltipWidget_->hide();

    this->unpause(PauseReason::Mouse);
}

bool ChannelView::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
    {
        if (const auto *gestureEvent = dynamic_cast<QGestureEvent *>(event))
        {
            return this->gestureEvent(gestureEvent);
        }
    }

    return BaseWidget::event(event);
}

bool ChannelView::gestureEvent(const QGestureEvent *event)
{
    if (QGesture *pan = event->gesture(Qt::PanGesture))
    {
        if (const auto *gesture = dynamic_cast<QPanGesture *>(pan))
        {
            switch (gesture->state())
            {
                case Qt::GestureStarted: {
                    this->isPanning_ = true;
                    // Remove any selections and hide tooltip while panning
                    this->clearSelection();
                    this->tooltipWidget_->hide();
                    if (this->isScrolling_)
                    {
                        this->disableScrolling();
                    }
                }
                break;

                case Qt::GestureUpdated: {
                    if (this->scrollBar_->isVisible())
                    {
                        this->scrollBar_->offset(-gesture->delta().y() * 0.1);
                    }
                }
                break;

                case Qt::GestureFinished:
                case Qt::GestureCanceled:
                default: {
                    this->clearSelection();
                    this->isPanning_ = false;
                }
                break;
            }

            return true;
        }
    }

    return false;
}

void ChannelView::mouseMoveEvent(QMouseEvent *event)
{
    if (this->isPanning_)
    {
        // Don't do any text selection, hovering, etc while panning
        return;
    }

    /// Pause on hover
    if (float pauseTime = getSettings()->pauseOnHoverDuration;
        pauseTime > 0.001F)
    {
        this->pause(PauseReason::Mouse,
                    static_cast<uint32_t>(pauseTime * 1000.F));
    }
    else if (pauseTime < -0.5F)
    {
        this->pause(PauseReason::Mouse);
    }

    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    // no message under cursor
    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex))
    {
        this->setCursor(Qt::ArrowCursor);
        this->tooltipWidget_->hide();
        return;
    }

    if (this->isScrolling_)
    {
        this->currentMousePosition_ = event->screenPos();
    }

    // check for word underneath cursor
    const MessageLayoutElement *hoverLayoutElement =
        layout->getElementAt(relativePos);

    // selecting single characters
    if (this->isLeftMouseDown_)
    {
        auto index = layout->getSelectionIndex(relativePos);
        this->setSelection(this->selection_.start,
                           SelectionItem(messageIndex, index));
    }

    // selecting whole words
    if (this->isDoubleClick_ && hoverLayoutElement)
    {
        auto [wordStart, wordEnd] =
            layout->getWordBounds(hoverLayoutElement, relativePos);
        auto hoveredWord = Selection{SelectionItem(messageIndex, wordStart),
                                     SelectionItem(messageIndex, wordEnd)};
        // combined selection spanning from initially selected word to hoveredWord
        auto selectUnion = this->doubleClickSelection_ | hoveredWord;

        this->setSelection(selectUnion);
    }

    // message under cursor is collapsed
    if (layout->flags.has(MessageLayoutFlag::Collapsed))
    {
        this->setCursor(Qt::PointingHandCursor);
        this->tooltipWidget_->hide();
        return;
    }

    if (hoverLayoutElement == nullptr)
    {
        this->setCursor(Qt::ArrowCursor);
        this->tooltipWidget_->hide();
        return;
    }

    auto *element = &hoverLayoutElement->getCreator();
    bool isLinkValid = hoverLayoutElement->getLink().isValid();
    const auto *emoteElement = dynamic_cast<const EmoteElement *>(element);
    const auto *layeredEmoteElement =
        dynamic_cast<const LayeredEmoteElement *>(element);
    bool isNotEmote = emoteElement == nullptr && layeredEmoteElement == nullptr;

    if (element->getTooltip().isEmpty() ||
        (isLinkValid && isNotEmote && !getSettings()->linkInfoTooltip))
    {
        this->tooltipWidget_->hide();
    }
    else
    {
        const auto *badgeElement = dynamic_cast<const BadgeElement *>(element);

        if (badgeElement || emoteElement || layeredEmoteElement)
        {
            auto showThumbnailSetting =
                getSettings()->emotesTooltipPreview.getValue();

            bool showThumbnail =
                showThumbnailSetting == ThumbnailPreviewMode::AlwaysShow ||
                (showThumbnailSetting == ThumbnailPreviewMode::ShowOnShift &&
                 event->modifiers() == Qt::ShiftModifier);

            if (emoteElement)
            {
                this->tooltipWidget_->setOne({
                    showThumbnail
                        ? emoteElement->getEmote()->images.getImage(3.0)
                        : nullptr,
                    element->getTooltip(),
                });
            }
            else if (layeredEmoteElement)
            {
                const auto &layeredEmotes = layeredEmoteElement->getEmotes();
                // Should never be empty but ensure it
                if (!layeredEmotes.empty())
                {
                    std::vector<TooltipEntry> entries;
                    entries.reserve(layeredEmotes.size());

                    const auto &emoteTooltips =
                        layeredEmoteElement->getEmoteTooltips();

                    // Someone performing some tomfoolery could put an emote with tens,
                    // if not hundreds of zero-width emotes on a single emote. If the
                    // tooltip may take up more than three rows, truncate everything else.
                    bool truncating = false;
                    size_t upperLimit = layeredEmotes.size();
                    if (layeredEmotes.size() > TOOLTIP_EMOTE_ENTRIES_LIMIT)
                    {
                        upperLimit = TOOLTIP_EMOTE_ENTRIES_LIMIT - 1;
                        truncating = true;
                    }

                    for (size_t i = 0; i < upperLimit; ++i)
                    {
                        const auto &emote = layeredEmotes[i].ptr;
                        if (i == 0)
                        {
                            // First entry gets a large image and full description
                            entries.push_back({showThumbnail
                                                   ? emote->images.getImage(3.0)
                                                   : nullptr,
                                               emoteTooltips[i]});
                        }
                        else
                        {
                            // Every other entry gets a small image and just the emote name
                            entries.push_back({showThumbnail
                                                   ? emote->images.getImage(1.0)
                                                   : nullptr,
                                               emote->name.string});
                        }
                    }

                    if (truncating)
                    {
                        entries.push_back({nullptr, "..."});
                    }

                    auto style = layeredEmotes.size() > 2
                                     ? TooltipStyle::Grid
                                     : TooltipStyle::Vertical;
                    this->tooltipWidget_->set(entries, style);
                }
            }
            else if (badgeElement)
            {
                this->tooltipWidget_->setOne({
                    showThumbnail
                        ? badgeElement->getEmote()->images.getImage(3.0)
                        : nullptr,
                    element->getTooltip(),
                });
            }
        }
        else
        {
            auto thumbnailSize = getSettings()->thumbnailSize;
            auto *linkElement = dynamic_cast<LinkElement *>(element);
            if (linkElement)
            {
                if (linkElement->linkInfo()->isPending())
                {
                    getApp()->getLinkResolver()->resolve(
                        linkElement->linkInfo());
                }
                this->setLinkInfoTooltip(linkElement->linkInfo());
            }
        }

        this->tooltipWidget_->moveTo(event->globalPos() + QPoint(16, 16),
                                     widgets::BoundsChecking::CursorPosition);
        this->tooltipWidget_->setWordWrap(isLinkValid);
        this->tooltipWidget_->show();
    }

    // check if word has a link
    if (isLinkValid)
    {
        this->setCursor(Qt::PointingHandCursor);
    }
    else
    {
        this->setCursor(Qt::ArrowCursor);
    }
}

void ChannelView::mousePressEvent(QMouseEvent *event)
{
    this->mouseDown.invoke(event);

    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex))
    {
        setCursor(Qt::ArrowCursor);
        auto &messagesSnapshot = this->getMessagesSnapshot();
        if (messagesSnapshot.size() == 0)
        {
            return;
        }

        // Start selection at the last message at its last index
        if (event->button() == Qt::LeftButton)
        {
            auto lastMessageIndex = messagesSnapshot.size() - 1;
            auto lastMessage = messagesSnapshot[lastMessageIndex];
            auto lastCharacterIndex = lastMessage->getLastCharacterIndex();

            SelectionItem selectionItem(lastMessageIndex, lastCharacterIndex);
            this->setSelection(selectionItem, selectionItem);
        }
        return;
    }

    // check if message is collapsed
    switch (event->button())
    {
        case Qt::LeftButton: {
            if (this->isScrolling_)
            {
                this->disableScrolling();
            }

            this->lastLeftPressPosition_ = event->screenPos();
            this->isLeftMouseDown_ = true;

            if (layout->flags.has(MessageLayoutFlag::Collapsed))
            {
                return;
            }

            if (getSettings()->linksDoubleClickOnly.getValue())
            {
                this->pause(PauseReason::DoubleClick, 200);
            }

            int index = layout->getSelectionIndex(relativePos);
            auto selectionItem = SelectionItem(messageIndex, index);
            this->setSelection(selectionItem, selectionItem);
        }
        break;

        case Qt::RightButton: {
            if (this->isScrolling_)
            {
                this->disableScrolling();
            }

            this->lastRightPressPosition_ = event->screenPos();
            this->isRightMouseDown_ = true;
        }
        break;

        case Qt::MiddleButton: {
            const MessageLayoutElement *hoverLayoutElement =
                layout->getElementAt(relativePos);

            if (hoverLayoutElement != nullptr &&
                hoverLayoutElement->getLink().isUrl() &&
                this->isScrolling_ == false)
            {
                break;
            }
            else
            {
                if (this->isScrolling_)
                {
                    this->disableScrolling();
                }
                else if (hoverLayoutElement != nullptr &&
                         hoverLayoutElement->getFlags().has(
                             MessageElementFlag::Username))
                {
                    break;
                }
                else if (this->scrollBar_->isVisible())
                {
                    this->enableScrolling(event->screenPos());
                }
            }
        }
        break;

        default:;
    }

    this->update();
}

void ChannelView::mouseReleaseEvent(QMouseEvent *event)
{
    // find message
    this->queueLayout();

    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    bool foundElement =
        tryGetMessageAt(event->pos(), layout, relativePos, messageIndex);

    // check if mouse was pressed
    if (event->button() == Qt::LeftButton)
    {
        if (this->isDoubleClick_)
        {
            this->isDoubleClick_ = false;
            // Was actually not a wanted triple-click
            if (std::abs(distanceBetweenPoints(this->lastDoubleClickPosition_,
                                               event->screenPos())) > 10.F)
            {
                this->clickTimer_.stop();
                return;
            }
        }
        else if (this->isLeftMouseDown_)
        {
            this->isLeftMouseDown_ = false;

            if (std::abs(distanceBetweenPoints(this->lastLeftPressPosition_,
                                               event->screenPos())) > 15.F)
            {
                return;
            }

            // Triple-clicking a message selects the whole message
            if (foundElement && this->clickTimer_.isActive() &&
                (std::abs(distanceBetweenPoints(this->lastDoubleClickPosition_,
                                                event->screenPos())) < 10.F))
            {
                this->selectWholeMessage(layout.get(), messageIndex);
                return;
            }
        }
        else
        {
            return;
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        if (this->isRightMouseDown_)
        {
            this->isRightMouseDown_ = false;

            if (std::abs(distanceBetweenPoints(this->lastRightPressPosition_,
                                               event->screenPos())) > 15.F)
            {
                return;
            }
        }
        else
        {
            return;
        }
    }
    else if (event->button() == Qt::MiddleButton)
    {
        if (this->isScrolling_ && this->scrollBar_->isVisible())
        {
            if (event->screenPos() == this->lastMiddlePressPosition_)
            {
                this->enableScrolling(event->screenPos());
            }
            else
            {
                this->disableScrolling();
            }

            return;
        }

        if (foundElement)
        {
            const MessageLayoutElement *hoverLayoutElement =
                layout->getElementAt(relativePos);

            if (hoverLayoutElement == nullptr)
            {
                return;
            }
            if (hoverLayoutElement->getFlags().has(
                    MessageElementFlag::Username))
            {
                openTwitchUsercard(this->channel_->getName(),
                                   hoverLayoutElement->getLink().value);
                return;
            }
            if (hoverLayoutElement->getLink().isUrl() == false)
            {
                return;
            }
        }
    }
    else
    {
        // not left or right button
        return;
    }

    // no message found
    if (!foundElement)
    {
        // No message at clicked position
        return;
    }

    // message under cursor is collapsed
    if (layout->flags.has(MessageLayoutFlag::Collapsed))
    {
        layout->flags.set(MessageLayoutFlag::Expanded);
        layout->flags.set(MessageLayoutFlag::RequiresLayout);

        this->queueLayout();
        return;
    }

    const MessageLayoutElement *hoverLayoutElement =
        layout->getElementAt(relativePos);

    // handle the click
    this->handleMouseClick(event, hoverLayoutElement, layout);

    this->update();
}

void ChannelView::handleMouseClick(QMouseEvent *event,
                                   const MessageLayoutElement *hoveredElement,
                                   MessageLayoutPtr layout)
{
    switch (event->button())
    {
        case Qt::LeftButton: {
            if (hoveredElement == nullptr)
            {
                return;
            }

            const auto &link = hoveredElement->getLink();
            if (!getSettings()->linksDoubleClickOnly)
            {
                this->handleLinkClick(event, link, layout.get());
            }

            // Invoke to signal from EmotePopup.
            if (link.type == Link::InsertText)
            {
                this->linkClicked.invoke(link);
            }
        }
        break;
        case Qt::RightButton: {
            // insert user mention to input, only in default context
            if ((this->context_ == Context::None) &&
                (hoveredElement != nullptr))
            {
                auto *split = dynamic_cast<Split *>(this->parentWidget());
                auto insertText = [=](QString text) {
                    if (split)
                    {
                        split->insertTextToInput(text);
                    }
                };
                const auto &link = hoveredElement->getLink();

                if (link.type == Link::UserInfo)
                {
                    // This is terrible because it FPs on messages where the
                    // user mentions themselves
                    bool canReply =
                        QString::compare(link.value,
                                         layout->getMessage()->loginName,
                                         Qt::CaseInsensitive) == 0;
                    UsernameRightClickBehavior action =
                        UsernameRightClickBehavior::Mention;
                    if (canReply)
                    {
                        Qt::KeyboardModifier userSpecifiedModifier =
                            getSettings()->usernameRightClickModifier;

                        if (userSpecifiedModifier ==
                            Qt::KeyboardModifier::NoModifier)
                        {
                            qCWarning(chatterinoCommon)
                                << "sanity check failed: "
                                   "invalid settings detected "
                                   "Settings::usernameRightClickModifier is "
                                   "NoModifier, which should never happen";
                            return;
                        }

                        Qt::KeyboardModifiers modifiers{userSpecifiedModifier};
                        auto isModifierHeld = event->modifiers() == modifiers;

                        if (isModifierHeld)
                        {
                            action = getSettings()
                                         ->usernameRightClickModifierBehavior;
                        }
                        else
                        {
                            action = getSettings()->usernameRightClickBehavior;
                        }
                    }
                    switch (action)
                    {
                        case UsernameRightClickBehavior::Mention: {
                            if (split == nullptr)
                            {
                                return;
                            }

                            if (link.value.startsWith("id:"))
                            {
                                return;
                            }

                            // Insert @username into split input
                            const bool commaMention =
                                getSettings()->mentionUsersWithComma;
                            const bool isFirstWord =
                                split->getInput().isEditFirstWord();
                            auto userMention = formatUserMention(
                                link.value, isFirstWord, commaMention);
                            insertText("@" + userMention + " ");
                        }
                        break;

                        case UsernameRightClickBehavior::Reply: {
                            // Start a new reply if matching user's settings
                            this->setInputReply(layout->getMessagePtr());
                        }
                        break;

                        case UsernameRightClickBehavior::Ignore:
                            break;

                        default: {
                            qCWarning(chatterinoCommon)
                                << "unhandled or corrupted "
                                   "UsernameRightClickBehavior value in "
                                   "ChannelView::handleMouseClick:"
                                << action;
                        }
                        break;  // unreachable
                    }

                    return;
                }

                if (link.type == Link::UserWhisper)
                {
                    insertText("/w " + link.value + " ");
                    return;
                }
            }

            this->addContextMenuItems(hoveredElement, layout, event);
        }
        break;
        case Qt::MiddleButton: {
            if (hoveredElement == nullptr)
            {
                return;
            }

            const auto &link = hoveredElement->getLink();
            if (!getSettings()->linksDoubleClickOnly)
            {
                this->handleLinkClick(event, link, layout.get());
            }
        }
        break;
        default:;
    }
}

void ChannelView::addContextMenuItems(
    const MessageLayoutElement *hoveredElement, MessageLayoutPtr layout,
    QMouseEvent *event)
{
    auto *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    // Add image options if the element clicked contains an image (e.g. a badge or an emote)
    addImageContextMenuItems(menu, hoveredElement);

    // Add link options if the element clicked contains a link
    addLinkContextMenuItems(menu, hoveredElement);

    // Add message options
    this->addMessageContextMenuItems(menu, layout);

    // Add Twitch-specific link options if the element clicked contains a link detected as a Twitch username
    this->addTwitchLinkContextMenuItems(menu, hoveredElement);

    // Add hidden options (e.g. copy message ID) if the user held down Shift
    addHiddenContextMenuItems(menu, hoveredElement, layout, event);

    // Add executable command options
    this->addCommandExecutionContextMenuItems(menu, layout);

    menu->popup(QCursor::pos());
    menu->raise();
}

void ChannelView::addMessageContextMenuItems(QMenu *menu,
                                             const MessageLayoutPtr &layout)
{
    // Copy actions
    if (!this->selection_.isEmpty())
    {
        menu->addAction("&Copy selection", [this] {
            crossPlatformCopy(this->getSelectedText());
        });
    }

    menu->addAction("Copy &message", [layout] {
        QString copyString;
        layout->addSelectionText(copyString, 0, INT_MAX,
                                 CopyMode::OnlyTextAndEmotes);

        crossPlatformCopy(copyString);
    });

    menu->addAction("Copy &full message", [layout] {
        QString copyString;
        layout->addSelectionText(copyString, 0, INT_MAX,
                                 CopyMode::EverythingButReplies);

        crossPlatformCopy(copyString);
    });

    // Only display reply option where it makes sense
    if (this->canReplyToMessages() && layout->isReplyable())
    {
        const auto &messagePtr = layout->getMessagePtr();
        menu->addAction("&Reply to message", [this, &messagePtr] {
            this->setInputReply(messagePtr);
        });

        if (messagePtr->replyThread != nullptr)
        {
            menu->addAction("Reply to &original thread", [this, &messagePtr] {
                this->setInputReply(messagePtr->replyThread->root());
            });

            menu->addAction("View &thread", [this, &messagePtr] {
                this->showReplyThreadPopup(messagePtr);
            });
        }
    }

    auto *twitchChannel =
        dynamic_cast<TwitchChannel *>(this->underlyingChannel_.get());
    if (!layout->getMessage()->id.isEmpty() && twitchChannel &&
        twitchChannel->hasModRights())
    {
        menu->addAction(
            "&Delete message", [twitchChannel, id = layout->getMessage()->id] {
                twitchChannel->deleteMessagesAs(
                    id, getApp()->getAccounts()->twitch.getCurrent().get());
            });
    }

    bool isSearch = this->context_ == Context::Search;
    bool isReplyOrUserCard = (this->context_ == Context::ReplyThread ||
                              this->context_ == Context::UserCard) &&
                             this->split_ != nullptr;
    bool isMentions =
        this->channel()->getType() == Channel::Type::TwitchMentions;
    bool isAutomod = this->channel()->getType() == Channel::Type::TwitchAutomod;
    if (isSearch || isMentions || isReplyOrUserCard || isAutomod)
    {
        const auto &messagePtr = layout->getMessagePtr();
        menu->addAction("&Go to message", [this, &messagePtr, isSearch,
                                           isMentions, isReplyOrUserCard,
                                           isAutomod] {
            if (isSearch)
            {
                if (const auto &search =
                        dynamic_cast<SearchPopup *>(this->parentWidget()))
                {
                    search->goToMessage(messagePtr);
                }
            }
            else if (isMentions || isAutomod)
            {
                getApp()->getWindows()->scrollToMessage(messagePtr);
            }
            else if (isReplyOrUserCard)
            {
                // If the thread is in the mentions or automod channel,
                // we need to find the original split.
                const auto type = this->split_->getChannel()->getType();
                if (type == Channel::Type::TwitchMentions ||
                    type == Channel::Type::TwitchAutomod)
                {
                    getApp()->getWindows()->scrollToMessage(messagePtr);
                }
                else
                {
                    this->split_->getChannelView().scrollToMessage(messagePtr);
                }
            }
        });
    }
}

void ChannelView::addTwitchLinkContextMenuItems(
    QMenu *menu, const MessageLayoutElement *hoveredElement)
{
    if (hoveredElement == nullptr)
    {
        return;
    }

    const auto &link = hoveredElement->getLink();

    if (link.type != Link::Url)
    {
        return;
    }

    static QRegularExpression twitchChannelRegex(
        R"(^(?:https?:\/\/)?(?:www\.|go\.)?twitch\.tv\/(?:popout\/)?(?<username>[a-z0-9_]{3,}))",
        QRegularExpression::CaseInsensitiveOption);
    static QSet<QString> ignoredUsernames{
        "directory",      //
        "downloads",      //
        "drops",          //
        "friends",        //
        "inventory",      //
        "jobs",           //
        "login",          //
        "messages",       //
        "payments",       //
        "profile",        //
        "security",       //
        "settings",       //
        "signup",         //
        "subscriptions",  //
        "turbo",          //
        "videos",         //
        "wallet",         //
    };

    auto twitchMatch = twitchChannelRegex.match(link.value);
    auto twitchUsername = twitchMatch.captured("username");
    if (!twitchUsername.isEmpty() && !ignoredUsernames.contains(twitchUsername))
    {
        menu->addSeparator();
        menu->addAction("&Open in new split", [twitchUsername, this] {
            this->openChannelIn.invoke(twitchUsername,
                                       FromTwitchLinkOpenChannelIn::Split);
        });
        menu->addAction("Open in new &tab", [twitchUsername, this] {
            this->openChannelIn.invoke(twitchUsername,
                                       FromTwitchLinkOpenChannelIn::Tab);
        });

        menu->addSeparator();
        menu->addAction("Open player in &browser", [twitchUsername, this] {
            this->openChannelIn.invoke(
                twitchUsername, FromTwitchLinkOpenChannelIn::BrowserPlayer);
        });
        menu->addAction("Open in &streamlink", [twitchUsername, this] {
            this->openChannelIn.invoke(twitchUsername,
                                       FromTwitchLinkOpenChannelIn::Streamlink);
        });
    }
}

void ChannelView::addCommandExecutionContextMenuItems(
    QMenu *menu, const MessageLayoutPtr &layout)
{
    /* Get commands to be displayed in context menu; 
     * only those that had the showInMsgContextMenu check box marked in the Commands page */
    std::vector<Command> cmds;
    for (const auto &cmd : getApp()->getCommands()->items)
    {
        if (cmd.showInMsgContextMenu)
        {
            cmds.push_back(cmd);
        }
    }

    if (cmds.empty())
    {
        return;
    }

    menu->addSeparator();
    auto *executeAction = menu->addAction("&Execute command");
    auto *cmdMenu = new QMenu(menu);
    executeAction->setMenu(cmdMenu);

    for (auto &cmd : cmds)
    {
        QString inputText = this->selection_.isEmpty()
                                ? layout->getMessage()->messageText
                                : this->getSelectedText();

        inputText.push_front(cmd.name + " ");

        cmdMenu->addAction(cmd.name, [this, layout, cmd, inputText] {
            ChannelPtr channel;

            /* Search popups and user message history's underlyingChannels aren't of type TwitchChannel, but
             * we would still like to execute commands from them. Use their source channel instead if applicable. */
            if (this->hasSourceChannel())
            {
                channel = this->sourceChannel();
            }
            else
            {
                channel = this->underlyingChannel_;
            }
            auto *split = dynamic_cast<Split *>(this->parentWidget());
            QString userText;
            if (split)
            {
                userText = split->getInput().getInputText();
            }

            // Execute command through right-clicking a message -> Execute command
            QString value = getApp()->getCommands()->execCustomCommand(
                inputText.split(' '), cmd, true, channel, layout->getMessage(),
                {
                    {"input.text", userText},
                });

            value = getApp()->getCommands()->execCommand(value, channel, false);

            channel->sendMessage(value);
        });
    }
}

void ChannelView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex))
    {
        return;
    }

    this->isDoubleClick_ = true;
    this->lastDoubleClickPosition_ = event->screenPos();
    this->clickTimer_.start();

    // message under cursor is collapsed
    if (layout->flags.has(MessageLayoutFlag::Collapsed))
    {
        return;
    }

    const MessageLayoutElement *hoverLayoutElement =
        layout->getElementAt(relativePos);

    if (hoverLayoutElement == nullptr)
    {
        // XXX: this is duplicate work
        auto idx = layout->getSelectionIndex(relativePos);
        SelectionItem item(messageIndex, idx);
        this->doubleClickSelection_ = {item, item};
        return;
    }

    auto [wordStart, wordEnd] =
        layout->getWordBounds(hoverLayoutElement, relativePos);

    this->doubleClickSelection_ = {SelectionItem(messageIndex, wordStart),
                                   SelectionItem(messageIndex, wordEnd)};
    this->setSelection(this->doubleClickSelection_);

    if (getSettings()->linksDoubleClickOnly)
    {
        const auto &link = hoverLayoutElement->getLink();
        this->handleLinkClick(event, link, layout.get());
    }
}

void ChannelView::hideEvent(QHideEvent * /*event*/)
{
    for (const auto &layout : this->messagesOnScreen_)
    {
        layout->deleteBuffer();
    }

    this->messagesOnScreen_.clear();
}

void ChannelView::showUserInfoPopup(const QString &userName,
                                    QString alternativePopoutChannel)
{
    if (!this->split_)
    {
        qCWarning(chatterinoCommon)
            << "Tried to show user info for" << userName
            << "but the channel view doesn't belong to a split.";
        return;
    }

    auto *userPopup =
        new UserInfoPopup(getSettings()->autoCloseUserPopup, this->split_);

    auto contextChannel =
        getApp()->getTwitch()->getChannelOrEmpty(alternativePopoutChannel);
    auto openingChannel = this->hasSourceChannel() ? this->sourceChannel_
                                                   : this->underlyingChannel_;
    userPopup->setData(userName, contextChannel, openingChannel);

    QPoint offset(userPopup->width() / 3, userPopup->height() / 5);
    userPopup->moveTo(QCursor::pos() - offset,
                      widgets::BoundsChecking::CursorPosition);
    userPopup->show();
}

bool ChannelView::mayContainMessage(const MessagePtr &message)
{
    switch (this->channel()->getType())
    {
        case Channel::Type::Direct:
        case Channel::Type::Twitch:
        case Channel::Type::TwitchWatching:
            // XXX: system messages may not have the channel set
            return message->flags.has(MessageFlag::System) ||
                   this->channel()->getName() == message->channelName;
        case Channel::Type::TwitchWhispers:
            return message->flags.has(MessageFlag::Whisper);
        case Channel::Type::TwitchMentions:
            return message->flags.has(MessageFlag::Highlighted);
        case Channel::Type::TwitchLive:
            return message->flags.has(MessageFlag::System);
        case Channel::Type::TwitchAutomod:
            return message->flags.has(MessageFlag::AutoMod);
        case Channel::Type::TwitchEnd:  // TODO: not used?
        case Channel::Type::None:       // Unspecific
        case Channel::Type::Misc:       // Unspecific
            return true;
        default:
            return true;  // unreachable
    }
}

void ChannelView::handleLinkClick(QMouseEvent *event, const Link &link,
                                  MessageLayout *layout)
{
    if (event->button() != Qt::LeftButton &&
        event->button() != Qt::MiddleButton)
    {
        return;
    }

    switch (link.type)
    {
        case Link::UserWhisper:
        case Link::UserInfo: {
            auto user = link.value;
            this->showUserInfoPopup(user, layout->getMessage()->channelName);
        }
        break;

        case Link::Url: {
            if (getSettings()->openLinksIncognito && supportsIncognitoLinks())
            {
                openLinkIncognito(link.value);
            }
            else
            {
                QDesktopServices::openUrl(QUrl(link.value));
            }
        }
        break;

        case Link::UserAction: {
            QString value = link.value;

            ChannelPtr channel = this->underlyingChannel_;
            auto *searchPopup =
                dynamic_cast<SearchPopup *>(this->parentWidget());
            if (searchPopup != nullptr)
            {
                auto *split =
                    dynamic_cast<Split *>(searchPopup->parentWidget());
                if (split != nullptr)
                {
                    channel = split->getChannel();
                }
            }

            // Execute command clicking a moderator button
            value = getApp()->getCommands()->execCustomCommand(
                QStringList(), Command{"(modaction)", value}, true, channel,
                layout->getMessage());

            value = getApp()->getCommands()->execCommand(value, channel, false);

            channel->sendMessage(value);
        }
        break;

        case Link::AutoModAllow: {
            getApp()->getAccounts()->twitch.getCurrent()->autoModAllow(
                link.value, this->channel());
        }
        break;

        case Link::AutoModDeny: {
            getApp()->getAccounts()->twitch.getCurrent()->autoModDeny(
                link.value, this->channel());
        }
        break;

        case Link::OpenAccountsPage: {
            SettingsDialog::showDialog(this,
                                       SettingsDialogPreference::Accounts);
        }
        break;
        case Link::JumpToChannel: {
            // Get all currently open pages
            QList<SplitContainer *> openPages;

            auto &nb = getApp()->getWindows()->getMainWindow().getNotebook();
            for (int i = 0; i < nb.getPageCount(); ++i)
            {
                openPages.push_back(
                    static_cast<SplitContainer *>(nb.getPageAt(i)));
            }

            for (auto *page : openPages)
            {
                auto splits = page->getSplits();

                // Search for channel matching link in page/split container
                // TODO(zneix): Consider opening a channel if it's closed (?)
                auto it = std::find_if(
                    splits.begin(), splits.end(), [link](Split *split) {
                        return split->getChannel()->getName() == link.value;
                    });

                if (it != splits.end())
                {
                    // Select SplitContainer and Split itself where mention message was sent
                    // TODO(zneix): Try exploring ways of scrolling to a certain message as well
                    nb.select(page);

                    Split *split = *it;
                    page->setSelected(split);
                    break;
                }
            }
        }
        break;
        case Link::CopyToClipboard: {
            crossPlatformCopy(link.value);
        }
        break;
        case Link::Reconnect: {
            this->underlyingChannel_.get()->reconnect();
        }
        break;
        case Link::ReplyToMessage: {
            this->setInputReply(layout->getMessagePtr());
        }
        break;
        case Link::ViewThread: {
            this->showReplyThreadPopup(layout->getMessagePtr());
        }
        break;
        case Link::JumpToMessage: {
            if (this->context_ == Context::Search)
            {
                auto *search =
                    dynamic_cast<SearchPopup *>(this->parentWidget());
                if (search != nullptr)
                {
                    search->goToMessageId(link.value);
                }
                return;
            }

            this->scrollToMessageId(link.value);
        }
        break;

        default:;
    }
}

bool ChannelView::tryGetMessageAt(QPoint p,
                                  std::shared_ptr<MessageLayout> &_message,
                                  QPoint &relativePos, int &index)
{
    auto &messagesSnapshot = this->getMessagesSnapshot();

    const auto start = size_t(this->scrollBar_->getRelativeCurrentValue());

    if (start >= messagesSnapshot.size())
    {
        return false;
    }

    int y = -(messagesSnapshot[start]->getHeight() *
              (fmod(this->scrollBar_->getRelativeCurrentValue(), 1)));

    for (size_t i = start; i < messagesSnapshot.size(); ++i)
    {
        auto message = messagesSnapshot[i];

        if (p.y() < y + message->getHeight())
        {
            relativePos = QPoint(p.x(), p.y() - y);
            _message = message;
            index = i;
            return true;
        }

        y += message->getHeight();
    }

    return false;
}

int ChannelView::getLayoutWidth() const
{
    if (this->scrollBar_->isVisible())
    {
        return int(this->width() - SCROLLBAR_PADDING * this->scale());
    }

    return this->width();
}

void ChannelView::selectWholeMessage(MessageLayout *layout, int &messageIndex)
{
    SelectionItem msgStart(messageIndex,
                           layout->getFirstMessageCharacterIndex());
    SelectionItem msgEnd(messageIndex, layout->getLastCharacterIndex());
    this->setSelection(msgStart, msgEnd);
}

void ChannelView::enableScrolling(const QPointF &scrollStart)
{
    this->isScrolling_ = true;
    this->lastMiddlePressPosition_ = scrollStart;
    // The line below prevents a sudden jerk at the beginning
    this->currentMousePosition_ = scrollStart;

    this->scrollTimer_.start();

    if (!QGuiApplication::overrideCursor())
    {
        QGuiApplication::setOverrideCursor(this->cursors_.neutral);
    }
}

void ChannelView::disableScrolling()
{
    this->isScrolling_ = false;
    this->scrollTimer_.stop();
    QGuiApplication::restoreOverrideCursor();
}

void ChannelView::scrollUpdateRequested()
{
    const qreal dpi = this->devicePixelRatioF();
    const qreal delta = dpi * (this->currentMousePosition_.y() -
                               this->lastMiddlePressPosition_.y());
    const int cursorHeight = this->cursors_.neutral.pixmap().height();

    if (fabs(delta) <= cursorHeight * dpi)
    {
        /*
         * If within an area close to the initial position, don't do any
         * scrolling at all.
         */
        QGuiApplication::changeOverrideCursor(this->cursors_.neutral);
        return;
    }

    qreal offset;
    if (delta > 0)
    {
        QGuiApplication::changeOverrideCursor(this->cursors_.down);
        offset = delta - cursorHeight;
    }
    else
    {
        QGuiApplication::changeOverrideCursor(this->cursors_.up);
        offset = delta + cursorHeight;
    }

    // "Good" feeling multiplier found by trial-and-error
    const qreal multiplier(0.02);
    this->scrollBar_->offset(multiplier * offset);
}

void ChannelView::setInputReply(const MessagePtr &message)
{
    assertInGuiThread();

    if (message == nullptr || this->split_ == nullptr)
    {
        return;
    }

    if (!message->replyThread)
    {
        // Message did not already have a thread attached, try to find or create one
        auto *tc =
            dynamic_cast<TwitchChannel *>(this->underlyingChannel_.get());

        if (tc)
        {
            tc->getOrCreateThread(message);
        }
        else
        {
            qCWarning(chatterinoCommon) << "Failed to create new reply thread";
            // Unable to create new reply thread.
            // TODO(dnsge): Should probably notify user?
            return;
        }
    }

    this->split_->setInputReply(message);
}

void ChannelView::showReplyThreadPopup(const MessagePtr &message)
{
    if (message == nullptr || message->replyThread == nullptr)
    {
        return;
    }

    if (!this->split_)
    {
        qCWarning(chatterinoCommon)
            << "Tried to show reply thread popup but the "
               "channel view doesn't belong to a split.";
        return;
    }

    auto *popup =
        new ReplyThreadPopup(getSettings()->autoCloseThreadPopup, this->split_);

    popup->setThread(message->replyThread);

    QPoint offset(int(150 * this->scale()), int(70 * this->scale()));
    popup->showAndMoveTo(QCursor::pos() - offset,
                         widgets::BoundsChecking::CursorPosition);
    popup->giveFocus(Qt::MouseFocusReason);
}

ChannelView::Context ChannelView::getContext() const
{
    return this->context_;
}

bool ChannelView::canReplyToMessages() const
{
    if (this->context_ == ChannelView::Context::ReplyThread ||
        this->context_ == ChannelView::Context::Search)
    {
        return false;
    }

    assert(this->channel_ != nullptr);

    if (!this->channel_->isTwitchChannel())
    {
        return false;
    }

    if (this->channel_->getType() == Channel::Type::TwitchWhispers ||
        this->channel_->getType() == Channel::Type::TwitchLive)
    {
        return false;
    }

    return true;
}

void ChannelView::setLinkInfoTooltip(LinkInfo *info)
{
    assert(info);

    auto thumbnailSize = getSettings()->thumbnailSize;

    ImagePtr thumbnail;
    if (info->hasThumbnail() && thumbnailSize > 0)
    {
        if (getApp()->getStreamerMode()->isEnabled() &&
            getSettings()->streamerModeHideLinkThumbnails)
        {
            thumbnail = Image::fromResourcePixmap(getResources().streamerMode);
        }
        else
        {
            thumbnail = info->thumbnail();
        }
    }

    this->tooltipWidget_->setOne({
        .image = thumbnail,
        .text = info->tooltip(),
        .customWidth = thumbnailSize,
        .customHeight = thumbnailSize,
    });

    if (info->isLoaded())
    {
        this->pendingLinkInfo_.clear();
        return;  // Either resolved or errored (can't change anymore)
    }

    // listen to changes

    if (this->pendingLinkInfo_.data() == info)
    {
        return;  // same info - already registered
    }

    if (this->pendingLinkInfo_)
    {
        QObject::disconnect(this->pendingLinkInfo_.data(),
                            &LinkInfo::stateChanged, this, nullptr);
    }
    QObject::connect(info, &LinkInfo::stateChanged, this,
                     &ChannelView::pendingLinkInfoStateChanged);
    this->pendingLinkInfo_ = info;
}

void ChannelView::pendingLinkInfoStateChanged()
{
    if (!this->pendingLinkInfo_)
    {
        return;
    }
    this->setLinkInfoTooltip(this->pendingLinkInfo_.data());
    this->tooltipWidget_->applyLastBoundsCheck();
}

void ChannelView::updateID()
{
    if (!this->underlyingChannel_)
    {
        // cannot update
        return;
    }

    std::size_t seed = 0;
    auto first = qHash(this->underlyingChannel_->getName());
    auto second = qHash(this->getFilterIds());

    boost::hash_combine(seed, first);
    boost::hash_combine(seed, second);

    this->id_ = seed;
}

ChannelView::ChannelViewID ChannelView::getID() const
{
    return this->id_;
}

}  // namespace chatterino
