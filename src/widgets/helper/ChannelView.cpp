#include "ChannelView.hpp"

#include <QClipboard>
#include <QColor>
#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QEasingCurve>
#include <QGraphicsBlurEffect>
#include <QMessageBox>
#include <QPainter>
#include <QScreen>
#include <QVariantAnimation>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandController.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "providers/LinkResolver.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/TooltipPreviewImage.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clipboard.hpp"
#include "util/DistanceBetweenPoints.hpp"
#include "util/Helpers.hpp"
#include "util/IncognitoBrowser.hpp"
#include "util/StreamerMode.hpp"
#include "util/Twitch.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/ReplyThreadPopup.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/SearchPopup.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitInput.hpp"

#define DRAW_WIDTH (this->width())
#define SELECTION_RESUME_SCROLLING_MSG_THRESHOLD 3
#define CHAT_HOVER_PAUSE_DURATION 1000

namespace chatterino {
namespace {
    void addEmoteContextMenuItems(const Emote &emote,
                                  MessageElementFlags creatorFlags, QMenu &menu)
    {
        auto openAction = menu.addAction("Open");
        auto openMenu = new QMenu;
        openAction->setMenu(openMenu);

        auto copyAction = menu.addAction("Copy");
        auto copyMenu = new QMenu;
        copyAction->setMenu(copyMenu);

        // see if the QMenu actually gets destroyed
        QObject::connect(openMenu, &QMenu::destroyed, [] {
            QMessageBox(QMessageBox::Information, "xD", "the menu got deleted")
                .exec();
        });

        // Add copy and open links for 1x, 2x, 3x
        auto addImageLink = [&](const ImagePtr &image, char scale) {
            if (!image->isEmpty())
            {
                copyMenu->addAction(QString(scale) + "x link",
                                    [url = image->url()] {
                                        crossPlatformCopy(url.string);
                                    });
                openMenu->addAction(
                    QString(scale) + "x link", [url = image->url()] {
                        QDesktopServices::openUrl(QUrl(url.string));
                    });
            }
        };

        addImageLink(emote.images.getImage1(), '1');
        addImageLink(emote.images.getImage2(), '2');
        addImageLink(emote.images.getImage3(), '3');

        // Copy and open emote page link
        auto addPageLink = [&](const QString &name) {
            copyMenu->addSeparator();
            openMenu->addSeparator();

            copyMenu->addAction("Copy " + name + " emote link",
                                [url = emote.homePage] {
                                    crossPlatformCopy(url.string);
                                });
            openMenu->addAction("Open " + name + " emote link",
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

ChannelView::ChannelView(BaseWidget *parent, Split *split, Context context)
    : BaseWidget(parent)
    , split_(split)
    , scrollBar_(new Scrollbar(this))
    , highlightAnimation_(this)
    , context_(context)
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
        /// remove elements that are finite
        for (auto it = this->pauses_.begin(); it != this->pauses_.end();)
            it = it->second ? this->pauses_.erase(it) : ++it;

        this->updatePauses();
    });

    auto shortcut = new QShortcut(QKeySequence::StandardKey::Copy, this);
    QObject::connect(shortcut, &QShortcut::activated, [this] {
        crossPlatformCopy(this->getSelectedText());
    });

    this->clickTimer_ = new QTimer(this);
    this->clickTimer_->setSingleShot(true);
    this->clickTimer_->setInterval(500);

    this->scrollTimer_.setInterval(20);
    QObject::connect(&this->scrollTimer_, &QTimer::timeout, this,
                     &ChannelView::scrollUpdateRequested);

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
}

void ChannelView::initializeLayout()
{
    this->goToBottom_ = new EffectLabel(this, 0);
    this->goToBottom_->setStyleSheet(
        "background-color: rgba(0,0,0,0.66); color: #FFF;");
    this->goToBottom_->getLabel().setText("More messages below");
    this->goToBottom_->setVisible(false);

    QObject::connect(this->goToBottom_, &EffectLabel::leftClicked, this, [=] {
        QTimer::singleShot(180, [=] {
            this->scrollBar_->scrollToBottom(
                getSettings()->enableSmoothScrollingNewMessages.getValue());
        });
    });
}

void ChannelView::initializeScrollbar()
{
    this->scrollBar_->getCurrentValueChanged().connect([this] {
        this->performLayout(true);
        this->queueUpdate();
    });
}

void ChannelView::initializeSignals()
{
    this->signalHolder_.managedConnect(getApp()->windows->wordFlagsChanged,
                                       [this] {
                                           this->queueLayout();
                                           this->update();
                                       });

    getSettings()->showLastMessageIndicator.connect(
        [this](auto, auto) {
            this->update();
        },
        this->signalHolder_);

    this->signalHolder_.managedConnect(getApp()->windows->gifRepaintRequested,
                                       [&] {
                                           this->queueUpdate();
                                       });

    this->signalHolder_.managedConnect(
        getApp()->windows->layoutRequested, [&](Channel *channel) {
            if (this->isVisible() &&
                (channel == nullptr || this->channel_.get() == channel))
            {
                this->queueLayout();
            }
        });

    this->signalHolder_.managedConnect(getApp()->fonts->fontChanged, [this] {
        this->queueLayout();
    });
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

void ChannelView::pause(PauseReason reason, boost::optional<uint> msecs)
{
    if (msecs)
    {
        /// Msecs has a value
        auto timePoint =
            SteadyClock::now() + std::chrono::milliseconds(msecs.get());
        auto it = this->pauses_.find(reason);

        if (it == this->pauses_.end())
        {
            /// No value found so we insert a new one.
            this->pauses_[reason] = timePoint;
        }
        else
        {
            /// If the new time point is newer then we override.
            if (it->second && it->second.get() < timePoint)
                it->second = timePoint;
        }
    }
    else
    {
        /// Msecs is none -> pause is infinite.
        /// We just override the value.
        this->pauses_[reason] = boost::none;
    }

    this->updatePauses();
}

void ChannelView::unpause(PauseReason reason)
{
    /// Remove the value from the map
    this->pauses_.erase(reason);

    this->updatePauses();
}

void ChannelView::updatePauses()
{
    using namespace std::chrono;

    if (this->pauses_.empty())
    {
        this->unpaused();

        /// No pauses so we can stop the timer
        this->pauseEnd_ = boost::none;
        this->pauseTimer_.stop();

        this->scrollBar_->offset(this->pauseScrollOffset_);
        this->pauseScrollOffset_ = 0;

        this->queueLayout();
    }
    else if (std::any_of(this->pauses_.begin(), this->pauses_.end(),
                         [](auto &&value) {
                             return !value.second;
                         }))
    {
        /// Some of the pauses are infinite
        this->pauseEnd_ = boost::none;
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
                ->second.get();

        if (pauseEnd != this->pauseEnd_)
        {
            /// Start the timer
            this->pauseEnd_ = pauseEnd;
            this->pauseTimer_.start(
                duration_cast<milliseconds>(pauseEnd - SteadyClock::now()));
        }
    }
}

void ChannelView::unpaused()
{
    /// Move selection
    this->selection_.selectionMin.messageIndex -= this->pauseSelectionOffset_;
    this->selection_.selectionMax.messageIndex -= this->pauseSelectionOffset_;
    this->selection_.start.messageIndex -= this->pauseSelectionOffset_;
    this->selection_.end.messageIndex -= this->pauseSelectionOffset_;

    this->pauseSelectionOffset_ = 0;
}

void ChannelView::themeChangedEvent()
{
    BaseWidget::themeChangedEvent();

    this->setupHighlightAnimationColors();
    this->queueLayout();
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
        auto factor = this->qtFontScale();
#ifdef Q_OS_MACOS
        factor = scale * 80.f /
                 std::max<float>(
                     0.01, this->logicalDpiX() * this->devicePixelRatioF());
#endif
        this->goToBottom_->getLabel().setFont(
            getFonts()->getFont(FontStyle::UiMedium, factor));
    }
}

void ChannelView::queueUpdate()
{
    //    if (this->updateTimer.isActive()) {
    //        this->updateQueued = true;
    //        return;
    //    }

    //    this->repaint();

    this->update();

    //    this->updateTimer.start();
}

void ChannelView::queueLayout()
{
    //    if (!this->layoutCooldown->isActive()) {
    this->performLayout();

    //        this->layoutCooldown->start();
    //    } else {
    //        this->layoutQueued = true;
    //    }
}

void ChannelView::performLayout(bool causedByScrollbar)
{
    // BenchmarkGuard benchmark("layout");

    /// Get messages and check if there are at least 1
    auto &messages = this->getMessagesSnapshot();

    this->showingLatestMessages_ =
        this->scrollBar_->isAtBottom() ||
        (!this->scrollBar_->isVisible() && !causedByScrollbar);

    /// Layout visible messages
    this->layoutVisibleMessages(messages);

    /// Update scrollbar
    this->updateScrollbar(messages, causedByScrollbar);

    this->goToBottom_->setVisible(this->enableScrollingToBottom_ &&
                                  this->scrollBar_->isVisible() &&
                                  !this->scrollBar_->isAtBottom());
}

void ChannelView::layoutVisibleMessages(
    LimitedQueueSnapshot<MessageLayoutPtr> &messages)
{
    const auto start = size_t(this->scrollBar_->getCurrentValue());
    const auto layoutWidth = this->getLayoutWidth();
    const auto flags = this->getFlags();
    auto redrawRequired = false;

    if (messages.size() > start)
    {
        auto y = int(-(messages[start]->getHeight() *
                       (fmod(this->scrollBar_->getCurrentValue(), 1))));

        for (auto i = start; i < messages.size() && y <= this->height(); i++)
        {
            auto message = messages[i];

            redrawRequired |=
                message->layout(layoutWidth, this->scale(), flags);

            y += message->getHeight();
        }
    }

    if (redrawRequired)
        this->queueUpdate();
}

void ChannelView::updateScrollbar(
    LimitedQueueSnapshot<MessageLayoutPtr> &messages, bool causedByScrollbar)
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

        message->layout(layoutWidth, this->scale(), flags);

        h -= message->getHeight();

        if (h < 0)  // break condition
        {
            this->scrollBar_->setLargeChange(
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
        this->scrollBar_->setDesiredValue(0);
    }
    this->showScrollBar_ = showScrollbar;

    this->scrollBar_->setMaximum(messages.size());

    // If we were showing the latest messages and the scrollbar now wants to be
    // rendered, scroll to bottom
    if (this->enableScrollingToBottom_ && this->showingLatestMessages_ &&
        showScrollbar)
    {
        this->scrollBar_->scrollToBottom(
            // this->messageWasAdded &&
            getSettings()->enableSmoothScrollingNewMessages.getValue());
        this->messageWasAdded_ = false;
    }
}

void ChannelView::clearMessages()
{
    // Clear all stored messages in this chat widget
    this->messages_.clear();
    this->scrollBar_->clearHighlights();
    this->queueLayout();

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

    Selection _selection = this->selection_;

    if (_selection.isEmpty())
    {
        return result;
    }

    for (int msg = _selection.selectionMin.messageIndex;
         msg <= _selection.selectionMax.messageIndex; msg++)
    {
        MessageLayoutPtr layout = messagesSnapshot[msg];
        int from = msg == _selection.selectionMin.messageIndex
                       ? _selection.selectionMin.charIndex
                       : 0;
        int to = msg == _selection.selectionMax.messageIndex
                     ? _selection.selectionMax.charIndex
                     : layout->getLastCharacterIndex() + 1;

        layout->addSelectionText(result, from, to);
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

void ChannelView::setEnableScrollingToBottom(bool value)
{
    this->enableScrollingToBottom_ = value;
}

bool ChannelView::getEnableScrollingToBottom() const
{
    return this->enableScrollingToBottom_;
}

void ChannelView::setOverrideFlags(boost::optional<MessageElementFlags> value)
{
    this->overrideFlags_ = std::move(value);
}

const boost::optional<MessageElementFlags> &ChannelView::getOverrideFlags()
    const
{
    return this->overrideFlags_;
}

LimitedQueueSnapshot<MessageLayoutPtr> &ChannelView::getMessagesSnapshot()
{
    if (!this->paused() /*|| this->scrollBar_->isVisible()*/)
    {
        this->snapshot_ = this->messages_.getSnapshot();
    }

    return this->snapshot_;
}

ChannelPtr ChannelView::channel()
{
    return this->channel_;
}

bool ChannelView::showScrollbarHighlights() const
{
    return this->channel_->getType() != Channel::Type::TwitchMentions;
}

void ChannelView::setChannel(ChannelPtr underlyingChannel)
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
               boost::optional<MessageFlags> overridingFlags) {
            if (this->shouldIncludeMessage(message))
            {
                if (this->channel_->lastDate_ != QDate::currentDate())
                {
                    this->channel_->lastDate_ = QDate::currentDate();
                    auto msg = makeSystemMessage(
                        QLocale().toString(QDate::currentDate(),
                                           QLocale::LongFormat),
                        QTime(0, 0));
                    this->channel_->addMessage(msg);
                }
                // When the message was received in the underlyingChannel,
                // logging will be handled. Prevent duplications.
                if (overridingFlags)
                {
                    overridingFlags.get().set(MessageFlag::DoNotLog);
                }
                else
                {
                    overridingFlags = MessageFlags(message->flags);
                    overridingFlags.get().set(MessageFlag::DoNotLog);
                }

                this->channel_->addMessage(message, overridingFlags);
            }
        });

    this->channelConnections_.managedConnect(
        underlyingChannel->messagesAddedAtStart,
        [this](std::vector<MessagePtr> &messages) {
            std::vector<MessagePtr> filtered;
            std::copy_if(messages.begin(), messages.end(),
                         std::back_inserter(filtered), [this](MessagePtr msg) {
                             return this->shouldIncludeMessage(msg);
                         });

            if (!filtered.empty())
                this->channel_->addMessagesAtStart(filtered);
        });

    this->channelConnections_.managedConnect(
        underlyingChannel->messageReplaced,
        [this](size_t index, MessagePtr replacement) {
            if (this->shouldIncludeMessage(replacement))
                this->channel_->replaceMessage(index, replacement);
        });

    this->channelConnections_.managedConnect(
        underlyingChannel->filledInMessages, [this](const auto &messages) {
            std::vector<MessagePtr> filtered;
            filtered.reserve(messages.size());
            std::copy_if(messages.begin(), messages.end(),
                         std::back_inserter(filtered), [this](MessagePtr msg) {
                             return this->shouldIncludeMessage(msg);
                         });
            this->channel_->fillInMissingMessages(filtered);
        });

    //
    // Standard channel connections
    //

    // on new message
    this->channelConnections_.managedConnect(
        this->channel_->messageAppended,
        [this](MessagePtr &message,
               boost::optional<MessageFlags> overridingFlags) {
            this->messageAppended(message, std::move(overridingFlags));
        });

    this->channelConnections_.managedConnect(
        this->channel_->messagesAddedAtStart,
        [this](std::vector<MessagePtr> &messages) {
            this->messageAddedAtStart(messages);
        });

    // on message removed
    this->channelConnections_.managedConnect(
        this->channel_->messageRemovedFromStart, [this](MessagePtr &message) {
            this->messageRemoveFromStart(message);
        });

    // on message replaced
    this->channelConnections_.managedConnect(
        this->channel_->messageReplaced,
        [this](size_t index, MessagePtr replacement) {
            this->messageReplaced(index, replacement);
        });

    // on messages filled in
    this->channelConnections_.managedConnect(this->channel_->filledInMessages,
                                             [this](const auto &) {
                                                 this->messagesUpdated();
                                             });

    auto snapshot = underlyingChannel->getMessageSnapshot();

    for (const auto &msg : snapshot)
    {
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
        if (this->showScrollbarHighlights())
        {
            this->scrollBar_->addHighlight(msg->getScrollBarHighlight());
        }
    }

    this->underlyingChannel_ = underlyingChannel;

    this->queueLayout();
    this->queueUpdate();

    // Notifications
    if (auto tc = dynamic_cast<TwitchChannel *>(underlyingChannel.get()))
    {
        this->channelConnections_.managedConnect(
            tc->liveStatusChanged, [this]() {
                this->liveStatusChanged.invoke();
            });
    }
}

void ChannelView::setFilters(const QList<QUuid> &ids)
{
    this->channelFilters_ = std::make_shared<FilterSet>(ids);
}

const QList<QUuid> ChannelView::getFilterIds() const
{
    if (!this->channelFilters_)
    {
        return QList<QUuid>();
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
            getApp()->accounts->twitch.getCurrent()->getUserName().compare(
                m->loginName, Qt::CaseInsensitive) == 0)
            return true;

        return this->channelFilters_->filter(m, this->channel_);
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
                                  boost::optional<MessageFlags> overridingFlags)
{
    auto *messageFlags = &message->flags;
    if (overridingFlags)
    {
        messageFlags = overridingFlags.get_ptr();
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

    if (!this->scrollBar_->isAtBottom() &&
        this->scrollBar_->getCurrentValueAnimation().state() ==
            QPropertyAnimation::Running)
    {
        QEventLoop loop;

        connect(&this->scrollBar_->getCurrentValueAnimation(),
                &QAbstractAnimation::stateChanged, &loop, &QEventLoop::quit);

        loop.exec();
    }

    if (this->messages_.pushBack(messageRef))
    {
        if (this->paused())
        {
            if (!this->scrollBar_->isAtBottom())
                this->pauseScrollOffset_--;
        }
        else
        {
            if (this->scrollBar_->isAtBottom())
                this->scrollBar_->scrollToBottom();
            else
                this->scrollBar_->offset(-1);
        }
    }

    if (!messageFlags->has(MessageFlag::DoNotTriggerNotification))
    {
        if (messageFlags->has(MessageFlag::Highlighted) &&
            messageFlags->has(MessageFlag::ShowInMentions) &&
            !messageFlags->has(MessageFlag::Subscription) &&
            (getSettings()->highlightMentions ||
             this->channel_->getType() != Channel::Type::TwitchMentions))

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

    this->messageWasAdded_ = true;
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
            layout->flags.set(MessageLayoutFlag::AlternateBackground);
        this->lastMessageHasAlternateBackgroundReverse_ =
            !this->lastMessageHasAlternateBackgroundReverse_;

        messageRefs.at(i) = std::move(layout);
    }

    /// Add the messages at the start
    if (this->messages_.pushFront(messageRefs).size() > 0)
    {
        if (this->scrollBar_->isAtBottom())
            this->scrollBar_->scrollToBottom();
        else
            this->scrollBar_->offset(qreal(messages.size()));
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

    this->messageWasAdded_ = true;
    this->queueLayout();
}

void ChannelView::messageRemoveFromStart(MessagePtr &message)
{
    if (this->paused())
    {
        this->pauseSelectionOffset_ += 1;
    }
    else
    {
        this->selection_.selectionMin.messageIndex--;
        this->selection_.selectionMax.messageIndex--;
        this->selection_.start.messageIndex--;
        this->selection_.end.messageIndex--;
    }

    this->queueLayout();
}

void ChannelView::messageReplaced(size_t index, MessagePtr &replacement)
{
    auto oMessage = this->messages_.get(index);
    if (!oMessage)
    {
        return;
    }

    auto message = *oMessage;

    auto newItem = std::make_shared<MessageLayout>(replacement);

    if (message->flags.has(MessageLayoutFlag::AlternateBackground))
    {
        newItem->flags.set(MessageLayoutFlag::AlternateBackground);
    }

    this->scrollBar_->replaceHighlight(index,
                                       replacement->getScrollBarHighlight());

    this->messages_.replaceItem(message, newItem);
    this->queueLayout();
}

void ChannelView::messagesUpdated()
{
    auto snapshot = this->channel_->getMessageSnapshot();

    this->messages_.clear();
    this->scrollBar_->clearHighlights();
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

void ChannelView::resizeEvent(QResizeEvent *)
{
    this->scrollBar_->setGeometry(this->width() - this->scrollBar_->width(), 0,
                                  this->scrollBar_->width(), this->height());

    this->goToBottom_->setGeometry(0, this->height() - int(this->scale() * 26),
                                   this->width(), int(this->scale() * 26));

    this->scrollBar_->raise();

    this->queueLayout();

    this->update();
}

void ChannelView::setSelection(const SelectionItem &start,
                               const SelectionItem &end)
{
    // selections
    if (!this->selecting_ && start != end)
    {
        // this->messagesAddedSinceSelectionPause_ = 0;

        this->selecting_ = true;
        // this->pausedBySelection_ = true;
    }

    this->selection_ = Selection(start, end);

    this->selectionChanged.invoke();
}

MessageElementFlags ChannelView::getFlags() const
{
    auto app = getApp();

    if (this->overrideFlags_)
    {
        return this->overrideFlags_.get();
    }

    MessageElementFlags flags = app->windows->getWordFlags();

    Split *split = dynamic_cast<Split *>(this->parentWidget());

    if (split == nullptr)
    {
        SearchPopup *searchPopup =
            dynamic_cast<SearchPopup *>(this->parentWidget());
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
        if (this->underlyingChannel_ == app->twitch->mentionsChannel ||
            this->underlyingChannel_ == app->twitch->liveChannel)
        {
            flags.set(MessageElementFlag::ChannelName);
            flags.unset(MessageElementFlag::ChannelPointReward);
        }
    }

    if (this->sourceChannel_ == app->twitch->mentionsChannel)
        flags.set(MessageElementFlag::ChannelName);

    if (this->context_ == Context::ReplyThread)
    {
        // Don't show inline replies within the ReplyThreadPopup
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
    getApp()->windows->select(this->split_);
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
    getApp()->windows->select(this->split_);
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
        this->getScrollBar().setDesiredValue(messageIdx);
    }
}

void ChannelView::paintEvent(QPaintEvent * /*event*/)
{
    //    BenchmarkGuard benchmark("paint");

    QPainter painter(this);

    painter.fillRect(rect(), this->theme->splits.background);

    // draw messages
    this->drawMessages(painter);

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
void ChannelView::drawMessages(QPainter &painter)
{
    auto &messagesSnapshot = this->getMessagesSnapshot();

    size_t start = size_t(this->scrollBar_->getCurrentValue());

    if (start >= messagesSnapshot.size())
    {
        return;
    }

    int y = int(-(messagesSnapshot[start].get()->getHeight() *
                  (fmod(this->scrollBar_->getCurrentValue(), 1))));

    MessageLayout *end = nullptr;
    bool windowFocused = this->window() == QApplication::activeWindow();

    auto app = getApp();
    bool isMentions = this->underlyingChannel_ == app->twitch->mentionsChannel;

    for (size_t i = start; i < messagesSnapshot.size(); ++i)
    {
        MessageLayout *layout = messagesSnapshot[i].get();

        bool isLastMessage = false;
        if (getSettings()->showLastMessageIndicator)
        {
            isLastMessage = this->lastReadMessage_.get() == layout;
        }

        layout->paint(painter, DRAW_WIDTH, y, i, this->selection_,
                      isLastMessage, windowFocused, isMentions);

        if (this->highlightedMessage_ == layout)
        {
            painter.fillRect(
                0, y, layout->getWidth(), layout->getHeight(),
                this->highlightAnimation_.currentValue().value<QColor>());
            if (this->highlightAnimation_.state() == QVariantAnimation::Stopped)
            {
                this->highlightedMessage_ = nullptr;
            }
        }

        y += layout->getHeight();

        end = layout;
        if (y > this->height())
        {
            break;
        }
    }

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
    if (!event->angleDelta().y())
    {
        return;
    }

    if (event->modifiers() & Qt::ControlModifier)
    {
        event->ignore();
        return;
    }

    if (this->scrollBar_->isVisible())
    {
        float mouseMultiplier = getSettings()->mouseScrollMultiplier;

        qreal desired = this->scrollBar_->getDesiredValue();
        qreal delta = event->angleDelta().y() * qreal(1.5) * mouseMultiplier;

        auto &snapshot = this->getMessagesSnapshot();
        int snapshotLength = int(snapshot.size());
        int i = std::min<int>(int(desired), snapshotLength);

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
                    snapshot[i - 1]->layout(this->getLayoutWidth(),
                                            this->scale(), this->getFlags());
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
                    snapshot[i + 1]->layout(this->getLayoutWidth(),
                                            this->scale(), this->getFlags());

                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i + 1]->getHeight();
                }
            }
        }

        this->scrollBar_->setDesiredValue(desired, true);
    }
}

void ChannelView::enterEvent(QEvent *)
{
}

void ChannelView::leaveEvent(QEvent *)
{
    TooltipWidget::instance()->hide();

    this->unpause(PauseReason::Mouse);

    this->queueLayout();
}

void ChannelView::mouseMoveEvent(QMouseEvent *event)
{
    /// Pause on hover
    if (float pauseTime = getSettings()->pauseOnHoverDuration;
        pauseTime > 0.001f)
    {
        this->pause(PauseReason::Mouse, uint(pauseTime * 1000.f));
    }
    else if (pauseTime < -0.5f)
    {
        this->pause(PauseReason::Mouse);
    }

    auto tooltipWidget = TooltipWidget::instance();
    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    // no message under cursor
    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex))
    {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }

    if (this->isScrolling_)
    {
        this->currentMousePosition_ = event->screenPos();
    }

    // is selecting
    if (this->isLeftMouseDown_)
    {
        // this->pause(PauseReason::Selecting, 300);
        int index = layout->getSelectionIndex(relativePos);

        this->setSelection(this->selection_.start,
                           SelectionItem(messageIndex, index));

        this->queueUpdate();
    }

    // message under cursor is collapsed
    if (layout->flags.has(MessageLayoutFlag::Collapsed))
    {
        this->setCursor(Qt::PointingHandCursor);
        tooltipWidget->hide();
        return;
    }

    // check if word underneath cursor
    const MessageLayoutElement *hoverLayoutElement =
        layout->getElementAt(relativePos);

    if (hoverLayoutElement == nullptr)
    {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }

    if (this->isDoubleClick_)
    {
        int wordStart;
        int wordEnd;
        this->getWordBounds(layout.get(), hoverLayoutElement, relativePos,
                            wordStart, wordEnd);
        SelectionItem newStart(messageIndex, wordStart);
        SelectionItem newEnd(messageIndex, wordEnd);

        // Selection changed in same message
        if (messageIndex == this->doubleClickSelection_.origMessageIndex)
        {
            // Selecting to the left
            if (wordStart < this->selection_.start.charIndex &&
                !this->doubleClickSelection_.selectingRight)
            {
                this->doubleClickSelection_.selectingLeft = true;
                // Ensure that the original word stays selected(Edge case)
                if (wordStart > this->doubleClickSelection_.originalEnd)
                {
                    this->setSelection(
                        this->doubleClickSelection_.origStartItem, newEnd);
                }
                else
                {
                    this->setSelection(newStart, this->selection_.end);
                }
                // Selecting to the right
            }
            else if (wordEnd > this->selection_.end.charIndex &&
                     !this->doubleClickSelection_.selectingLeft)
            {
                this->doubleClickSelection_.selectingRight = true;
                // Ensure that the original word stays selected(Edge case)
                if (wordEnd < this->doubleClickSelection_.originalStart)
                {
                    this->setSelection(newStart,
                                       this->doubleClickSelection_.origEndItem);
                }
                else
                {
                    this->setSelection(this->selection_.start, newEnd);
                }
            }
            // Swapping from selecting left to selecting right
            if (wordStart > this->selection_.start.charIndex &&
                !this->doubleClickSelection_.selectingRight)
            {
                if (wordStart > this->doubleClickSelection_.originalEnd)
                {
                    this->doubleClickSelection_.selectingLeft = false;
                    this->doubleClickSelection_.selectingRight = true;
                    this->setSelection(
                        this->doubleClickSelection_.origStartItem, newEnd);
                }
                else
                {
                    this->setSelection(newStart, this->selection_.end);
                }
                // Swapping from selecting right to selecting left
            }
            else if (wordEnd < this->selection_.end.charIndex &&
                     !this->doubleClickSelection_.selectingLeft)
            {
                if (wordEnd < this->doubleClickSelection_.originalStart)
                {
                    this->doubleClickSelection_.selectingLeft = true;
                    this->doubleClickSelection_.selectingRight = false;
                    this->setSelection(newStart,
                                       this->doubleClickSelection_.origEndItem);
                }
                else
                {
                    this->setSelection(this->selection_.start, newEnd);
                }
            }
            // Selection changed in a different message
        }
        else
        {
            // Message over the original
            if (messageIndex < this->selection_.start.messageIndex)
            {
                // Swapping from left to right selecting
                if (!this->doubleClickSelection_.selectingLeft)
                {
                    this->doubleClickSelection_.selectingLeft = true;
                    this->doubleClickSelection_.selectingRight = false;
                }
                if (wordStart < this->selection_.start.charIndex &&
                    !this->doubleClickSelection_.selectingRight)
                {
                    this->doubleClickSelection_.selectingLeft = true;
                }
                this->setSelection(newStart,
                                   this->doubleClickSelection_.origEndItem);
                // Message under the original
            }
            else if (messageIndex > this->selection_.end.messageIndex)
            {
                // Swapping from right to left selecting
                if (!this->doubleClickSelection_.selectingRight)
                {
                    this->doubleClickSelection_.selectingLeft = false;
                    this->doubleClickSelection_.selectingRight = true;
                }
                if (wordEnd > this->selection_.end.charIndex &&
                    !this->doubleClickSelection_.selectingLeft)
                {
                    this->doubleClickSelection_.selectingRight = true;
                }
                this->setSelection(this->doubleClickSelection_.origStartItem,
                                   newEnd);
                // Selection changed in non original message
            }
            else
            {
                if (this->doubleClickSelection_.selectingLeft)
                {
                    this->setSelection(newStart, this->selection_.end);
                }
                else
                {
                    this->setSelection(this->selection_.start, newEnd);
                }
            }
        }
        // Reset direction of selection
        if (wordStart == this->doubleClickSelection_.originalStart &&
            wordEnd == this->doubleClickSelection_.originalEnd)
        {
            this->doubleClickSelection_.selectingLeft =
                this->doubleClickSelection_.selectingRight = false;
        }
    }

    auto element = &hoverLayoutElement->getCreator();
    bool isLinkValid = hoverLayoutElement->getLink().isValid();
    auto emoteElement = dynamic_cast<const EmoteElement *>(element);

    if (element->getTooltip().isEmpty() ||
        (isLinkValid && emoteElement == nullptr &&
         !getSettings()->linkInfoTooltip))
    {
        tooltipWidget->hide();
    }
    else
    {
        auto &tooltipPreviewImage = TooltipPreviewImage::instance();
        tooltipPreviewImage.setImageScale(0, 0);
        auto badgeElement = dynamic_cast<const BadgeElement *>(element);

        if ((badgeElement || emoteElement) &&
            getSettings()->emotesTooltipPreview.getValue())
        {
            if (event->modifiers() == Qt::ShiftModifier ||
                getSettings()->emotesTooltipPreview.getValue() == 1)
            {
                if (emoteElement)
                {
                    tooltipPreviewImage.setImage(
                        emoteElement->getEmote()->images.getImage(3.0));
                }
                else if (badgeElement)
                {
                    tooltipPreviewImage.setImage(
                        badgeElement->getEmote()->images.getImage(3.0));
                }
            }
            else
            {
                tooltipPreviewImage.setImage(nullptr);
            }
        }
        else
        {
            if (element->getTooltip() == "No link info loaded")
            {
                std::weak_ptr<MessageLayout> weakLayout = layout;
                LinkResolver::getLinkInfo(
                    element->getLink().value, nullptr,
                    [weakLayout, element](QString tooltipText,
                                          Link originalLink,
                                          ImagePtr thumbnail) {
                        auto shared = weakLayout.lock();
                        if (!shared)
                            return;
                        element->setTooltip(tooltipText);
                        element->setThumbnail(thumbnail);
                    });
            }
            auto thumbnailSize = getSettings()->thumbnailSize;
            if (!thumbnailSize)
            {
                tooltipPreviewImage.setImage(nullptr);
            }
            else
            {
                const auto shouldHideThumbnail =
                    isInStreamerMode() &&
                    getSettings()->streamerModeHideLinkThumbnails &&
                    element->getThumbnail() != nullptr &&
                    !element->getThumbnail()->url().string.isEmpty();
                auto thumb =
                    shouldHideThumbnail
                        ? Image::fromResourcePixmap(getResources().streamerMode)
                        : element->getThumbnail();
                tooltipPreviewImage.setImage(std::move(thumb));

                if (element->getThumbnailType() ==
                    MessageElement::ThumbnailType::Link_Thumbnail)
                {
                    tooltipPreviewImage.setImageScale(thumbnailSize,
                                                      thumbnailSize);
                }
            }
        }

        tooltipWidget->moveTo(this, event->globalPos());
        tooltipWidget->setWordWrap(isLinkValid);
        tooltipWidget->setText(element->getTooltip());
        tooltipWidget->adjustSize();
        tooltipWidget->setWindowFlag(Qt::WindowStaysOnTopHint, true);
        tooltipWidget->show();
        tooltipWidget->raise();
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
                this->disableScrolling();

            this->lastLeftPressPosition_ = event->screenPos();
            this->isLeftMouseDown_ = true;

            if (layout->flags.has(MessageLayoutFlag::Collapsed))
                return;

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
                this->disableScrolling();

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
                    this->disableScrolling();
                else if (hoverLayoutElement != nullptr &&
                         hoverLayoutElement->getFlags().has(
                             MessageElementFlag::Username))
                    break;
                else if (this->scrollBar_->isVisible())
                    this->enableScrolling(event->screenPos());
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
        this->doubleClickSelection_.selectingLeft =
            this->doubleClickSelection_.selectingRight = false;
        if (this->isDoubleClick_)
        {
            this->isDoubleClick_ = false;
            // Was actually not a wanted triple-click
            if (fabsf(distanceBetweenPoints(this->lastDClickPosition_,
                                            event->screenPos())) > 10.f)
            {
                this->clickTimer_->stop();
                return;
            }
        }
        else if (this->isLeftMouseDown_)
        {
            this->isLeftMouseDown_ = false;

            if (fabsf(distanceBetweenPoints(this->lastLeftPressPosition_,
                                            event->screenPos())) > 15.f)
            {
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

            if (fabsf(distanceBetweenPoints(this->lastRightPressPosition_,
                                            event->screenPos())) > 15.f)
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
                this->enableScrolling(event->screenPos());
            else
                this->disableScrolling();

            return;
        }
        else if (foundElement)
        {
            const MessageLayoutElement *hoverLayoutElement =
                layout->getElementAt(relativePos);

            if (hoverLayoutElement == nullptr)
            {
                return;
            }
            else if (hoverLayoutElement->getFlags().has(
                         MessageElementFlag::Username))
            {
                openTwitchUsercard(this->channel_->getName(),
                                   hoverLayoutElement->getLink().value);
                return;
            }
            else if (hoverLayoutElement->getLink().isUrl() == false)
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
    // Triple-clicking a message selects the whole message
    if (this->clickTimer_->isActive() && this->selecting_)
    {
        if (fabsf(distanceBetweenPoints(this->lastDClickPosition_,
                                        event->screenPos())) < 10.f)
        {
            this->selectWholeMessage(layout.get(), messageIndex);
        }
    }

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
            if (this->selecting_)
            {
                // this->pausedBySelection = false;
                this->selecting_ = false;
                // this->pauseTimeout.stop();
                // this->pausedTemporarily = false;

                this->queueLayout();
            }

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
            auto split = dynamic_cast<Split *>(this->parentWidget());
            auto insertText = [=](QString text) {
                if (split)
                {
                    split->insertTextToInput(text);
                }
            };

            if (hoveredElement != nullptr)
            {
                const auto &link = hoveredElement->getLink();

                if (link.type == Link::UserInfo)
                {
                    const bool commaMention =
                        getSettings()->mentionUsersWithComma;
                    const bool isFirstWord =
                        split && split->getInput().isEditFirstWord();
                    auto userMention = formatUserMention(
                        link.value, isFirstWord, commaMention);
                    insertText("@" + userMention + " ");
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
    static QMenu *previousMenu = nullptr;
    if (previousMenu != nullptr)
    {
        previousMenu->deleteLater();
        previousMenu = nullptr;
    }

    auto menu = new QMenu;
    previousMenu = menu;

    // Add image options if the element clicked contains an image (e.g. a badge or an emote)
    this->addImageContextMenuItems(hoveredElement, layout, event, *menu);

    // Add link options if the element clicked contains a link
    this->addLinkContextMenuItems(hoveredElement, layout, event, *menu);

    // Add message options
    this->addMessageContextMenuItems(hoveredElement, layout, event, *menu);

    // Add Twitch-specific link options if the element clicked contains a link detected as a Twitch username
    this->addTwitchLinkContextMenuItems(hoveredElement, layout, event, *menu);

    // Add hidden options (e.g. copy message ID) if the user held down Shift
    this->addHiddenContextMenuItems(hoveredElement, layout, event, *menu);

    // Add executable command options
    this->addCommandExecutionContextMenuItems(hoveredElement, layout, event,
                                              *menu);

    menu->popup(QCursor::pos());
    menu->raise();
}

void ChannelView::addImageContextMenuItems(
    const MessageLayoutElement *hoveredElement, MessageLayoutPtr /*layout*/,
    QMouseEvent * /*event*/, QMenu &menu)
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
        if (auto badgeElement = dynamic_cast<const BadgeElement *>(&creator))
        {
            addEmoteContextMenuItems(*badgeElement->getEmote(), creatorFlags,
                                     menu);
        }
    }

    // Emote actions
    if (creatorFlags.hasAny(
            {MessageElementFlag::EmoteImages, MessageElementFlag::EmojiImage}))
    {
        if (auto emoteElement = dynamic_cast<const EmoteElement *>(&creator))
        {
            addEmoteContextMenuItems(*emoteElement->getEmote(), creatorFlags,
                                     menu);
        }
    }

    // add seperator
    if (!menu.actions().empty())
    {
        menu.addSeparator();
    }
}

void ChannelView::addLinkContextMenuItems(
    const MessageLayoutElement *hoveredElement, MessageLayoutPtr /*layout*/,
    QMouseEvent * /*event*/, QMenu &menu)
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
    menu.addAction("Open link", [url] {
        QDesktopServices::openUrl(QUrl(url));
    });
    // open link default
    if (supportsIncognitoLinks())
    {
        menu.addAction("Open link incognito", [url] {
            openLinkIncognito(url);
        });
    }
    menu.addAction("Copy link", [url] {
        crossPlatformCopy(url);
    });

    menu.addSeparator();
}
void ChannelView::addMessageContextMenuItems(
    const MessageLayoutElement * /*hoveredElement*/, MessageLayoutPtr layout,
    QMouseEvent * /*event*/, QMenu &menu)
{
    // Copy actions
    if (!this->selection_.isEmpty())
    {
        menu.addAction("Copy selection", [this] {
            crossPlatformCopy(this->getSelectedText());
        });
    }

    menu.addAction("Copy message", [layout] {
        QString copyString;
        layout->addSelectionText(copyString, 0, INT_MAX,
                                 CopyMode::OnlyTextAndEmotes);

        crossPlatformCopy(copyString);
    });

    menu.addAction("Copy full message", [layout] {
        QString copyString;
        layout->addSelectionText(copyString, 0, INT_MAX,
                                 CopyMode::EverythingButReplies);

        crossPlatformCopy(copyString);
    });

    // Only display reply option where it makes sense
    if (this->canReplyToMessages() && layout->isReplyable())
    {
        const auto &messagePtr = layout->getMessagePtr();
        menu.addAction("Reply to message", [this, &messagePtr] {
            this->setInputReply(messagePtr);
        });

        if (messagePtr->replyThread != nullptr)
        {
            menu.addAction("View thread", [this, &messagePtr] {
                this->showReplyThreadPopup(messagePtr);
            });
        }
    }

    bool isSearch = this->context_ == Context::Search;
    bool isReplyOrUserCard = (this->context_ == Context::ReplyThread ||
                              this->context_ == Context::UserCard) &&
                             this->split_;
    bool isMentions =
        this->channel()->getType() == Channel::Type::TwitchMentions;
    if (isSearch || isMentions || isReplyOrUserCard)
    {
        const auto &messagePtr = layout->getMessagePtr();
        menu.addAction("Go to message", [this, &messagePtr, isSearch,
                                         isMentions, isReplyOrUserCard] {
            if (isSearch)
            {
                if (const auto &search =
                        dynamic_cast<SearchPopup *>(this->parentWidget()))
                {
                    search->goToMessage(messagePtr);
                }
            }
            else if (isMentions)
            {
                getApp()->windows->scrollToMessage(messagePtr);
            }
            else if (isReplyOrUserCard)
            {
                // If the thread is in the mentions channel,
                // we need to find the original split.
                if (this->split_->getChannel()->getType() ==
                    Channel::Type::TwitchMentions)
                {
                    getApp()->windows->scrollToMessage(messagePtr);
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
    const MessageLayoutElement *hoveredElement, MessageLayoutPtr /*layout*/,
    QMouseEvent * /*event*/, QMenu &menu)
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
        menu.addSeparator();
        menu.addAction("Open in new split", [twitchUsername, this] {
            this->openChannelIn.invoke(twitchUsername,
                                       FromTwitchLinkOpenChannelIn::Split);
        });
        menu.addAction("Open in new tab", [twitchUsername, this] {
            this->openChannelIn.invoke(twitchUsername,
                                       FromTwitchLinkOpenChannelIn::Tab);
        });

        menu.addSeparator();
        menu.addAction("Open player in browser", [twitchUsername, this] {
            this->openChannelIn.invoke(
                twitchUsername, FromTwitchLinkOpenChannelIn::BrowserPlayer);
        });
        menu.addAction("Open in streamlink", [twitchUsername, this] {
            this->openChannelIn.invoke(twitchUsername,
                                       FromTwitchLinkOpenChannelIn::Streamlink);
        });
    }
}

void ChannelView::addHiddenContextMenuItems(
    const MessageLayoutElement * /*hoveredElement*/, MessageLayoutPtr layout,
    QMouseEvent *event, QMenu &menu)
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
        menu.addAction("Copy message ID",
                       [messageID = layout->getMessage()->id] {
                           crossPlatformCopy(messageID);
                       });
    }
}

void ChannelView::addCommandExecutionContextMenuItems(
    const MessageLayoutElement * /*hoveredElement*/, MessageLayoutPtr layout,
    QMouseEvent * /*event*/, QMenu &menu)
{
    /* Get commands to be displayed in context menu; 
     * only those that had the showInMsgContextMenu check box marked in the Commands page */
    std::vector<Command> cmds;
    for (auto &cmd : getApp()->commands->items)
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

    menu.addSeparator();
    auto executeAction = menu.addAction("Execute command");
    auto cmdMenu = new QMenu;
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
            auto split = dynamic_cast<Split *>(this->parentWidget());
            QString userText;
            if (split)
            {
                userText = split->getInput().getInputText();
            }

            // Execute command through right-clicking a message -> Execute command
            QString value = getApp()->commands->execCustomCommand(
                inputText.split(' '), cmd, true, channel, layout->getMessage(),
                {
                    {"input.text", userText},
                });

            value = getApp()->commands->execCommand(value, channel, false);

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

    // message under cursor is collapsed
    if (layout->flags.has(MessageLayoutFlag::Collapsed))
    {
        return;
    }

    const MessageLayoutElement *hoverLayoutElement =
        layout->getElementAt(relativePos);
    this->lastDClickPosition_ = event->screenPos();

    if (hoverLayoutElement == nullptr)
    {
        // Possibility for triple click which doesn't have to be over an
        // existing layout element
        this->clickTimer_->start();
        return;
    }

    if (!this->isLeftMouseDown_)
    {
        this->isDoubleClick_ = true;

        int wordStart;
        int wordEnd;
        this->getWordBounds(layout.get(), hoverLayoutElement, relativePos,
                            wordStart, wordEnd);

        this->clickTimer_->start();

        SelectionItem wordMin(messageIndex, wordStart);
        SelectionItem wordMax(messageIndex, wordEnd);

        this->doubleClickSelection_.originalStart = wordStart;
        this->doubleClickSelection_.originalEnd = wordEnd;
        this->doubleClickSelection_.origMessageIndex = messageIndex;
        this->doubleClickSelection_.origStartItem = wordMin;
        this->doubleClickSelection_.origEndItem = wordMax;

        this->setSelection(wordMin, wordMax);
    }

    if (getSettings()->linksDoubleClickOnly)
    {
        auto &link = hoverLayoutElement->getLink();
        this->handleLinkClick(event, link, layout.get());
    }
}

void ChannelView::hideEvent(QHideEvent *)
{
    for (auto &layout : this->messagesOnScreen_)
    {
        layout->deleteBuffer();
    }

    this->messagesOnScreen_.clear();
}

void ChannelView::showUserInfoPopup(const QString &userName,
                                    QString alternativePopoutChannel)
{
    auto *userCardParent =
        static_cast<QWidget *>(&(getApp()->windows->getMainWindow()));
    auto *userPopup = new UserInfoPopup(getSettings()->autoCloseUserPopup,
                                        userCardParent, this->split_);

    auto contextChannel =
        getApp()->twitch->getChannelOrEmpty(alternativePopoutChannel);
    auto openingChannel = this->hasSourceChannel() ? this->sourceChannel_
                                                   : this->underlyingChannel_;
    userPopup->setData(userName, contextChannel, openingChannel);

    QPoint offset(int(150 * this->scale()), int(70 * this->scale()));
    userPopup->move(QCursor::pos() - offset);
    userPopup->show();
}

bool ChannelView::mayContainMessage(const MessagePtr &message)
{
    switch (this->channel()->getType())
    {
        case Channel::Type::Direct:
        case Channel::Type::Twitch:
        case Channel::Type::TwitchWatching:
        case Channel::Type::Irc:
            return this->channel()->getName() == message->channelName;
        case Channel::Type::TwitchWhispers:
            return message->flags.has(MessageFlag::Whisper);
        case Channel::Type::TwitchMentions:
            return message->flags.has(MessageFlag::Highlighted);
        case Channel::Type::TwitchLive:
            return message->flags.has(MessageFlag::System);
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
                openLinkIncognito(link.value);
            else
                QDesktopServices::openUrl(QUrl(link.value));
        }
        break;

        case Link::UserAction: {
            QString value = link.value;

            ChannelPtr channel = this->underlyingChannel_;
            SearchPopup *searchPopup =
                dynamic_cast<SearchPopup *>(this->parentWidget());
            if (searchPopup != nullptr)
            {
                Split *split =
                    dynamic_cast<Split *>(searchPopup->parentWidget());
                if (split != nullptr)
                {
                    channel = split->getChannel();
                }
            }

            // Execute command clicking a moderator button
            value = getApp()->commands->execCustomCommand(
                QStringList(), Command{"(modaction)", value}, true, channel,
                layout->getMessage());

            value = getApp()->commands->execCommand(value, channel, false);

            channel->sendMessage(value);
        }
        break;

        case Link::AutoModAllow: {
            getApp()->accounts->twitch.getCurrent()->autoModAllow(
                link.value, this->channel());
        }
        break;

        case Link::AutoModDeny: {
            getApp()->accounts->twitch.getCurrent()->autoModDeny(
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

            auto &nb = getApp()->windows->getMainWindow().getNotebook();
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
                if (auto search =
                        dynamic_cast<SearchPopup *>(this->parentWidget()))
                {
                    search->goToMessageId(link.value);
                }
            }
            else
            {
                this->scrollToMessageId(link.value);
            }
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

    size_t start = this->scrollBar_->getCurrentValue();

    if (start >= messagesSnapshot.size())
    {
        return false;
    }

    int y = -(messagesSnapshot[start]->getHeight() *
              (fmod(this->scrollBar_->getCurrentValue(), 1)));

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
        return int(this->width() - scrollbarPadding * this->scale());

    return this->width();
}

void ChannelView::selectWholeMessage(MessageLayout *layout, int &messageIndex)
{
    SelectionItem msgStart(messageIndex,
                           layout->getFirstMessageCharacterIndex());
    SelectionItem msgEnd(messageIndex, layout->getLastCharacterIndex());
    this->setSelection(msgStart, msgEnd);
}

void ChannelView::getWordBounds(MessageLayout *layout,
                                const MessageLayoutElement *element,
                                const QPoint &relativePos, int &wordStart,
                                int &wordEnd)
{
    const int mouseInWordIndex = element->getMouseOverIndex(relativePos);
    wordStart = layout->getSelectionIndex(relativePos) - mouseInWordIndex;
    const int selectionLength = element->getSelectionIndexCount();
    const int length =
        element->hasTrailingSpace() ? selectionLength - 1 : selectionLength;
    wordEnd = wordStart + length;
}

void ChannelView::enableScrolling(const QPointF &scrollStart)
{
    this->isScrolling_ = true;
    this->lastMiddlePressPosition_ = scrollStart;
    // The line below prevents a sudden jerk at the beginning
    this->currentMousePosition_ = scrollStart;

    this->scrollTimer_.start();

    if (!QGuiApplication::overrideCursor())
        QGuiApplication::setOverrideCursor(this->cursors_.neutral);
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
    const qreal multiplier = qreal(0.02);
    this->scrollBar_->offset(multiplier * offset);
}

void ChannelView::setInputReply(const MessagePtr &message)
{
    if (message == nullptr)
    {
        return;
    }

    std::shared_ptr<MessageThread> thread;

    if (message->replyThread == nullptr)
    {
        auto getThread = [&](TwitchChannel *tc) {
            auto threadIt = tc->threads().find(message->id);
            if (threadIt != tc->threads().end() && !threadIt->second.expired())
            {
                return threadIt->second.lock();
            }
            else
            {
                auto thread = std::make_shared<MessageThread>(message);
                tc->addReplyThread(thread);
                return thread;
            }
        };

        if (auto tc =
                dynamic_cast<TwitchChannel *>(this->underlyingChannel_.get()))
        {
            thread = getThread(tc);
        }
        else if (auto tc = dynamic_cast<TwitchChannel *>(this->channel_.get()))
        {
            thread = getThread(tc);
        }
        else
        {
            qCWarning(chatterinoCommon) << "Failed to create new reply thread";
            // Unable to create new reply thread.
            // TODO(dnsge): Should probably notify user?
            return;
        }
    }
    else
    {
        thread = message->replyThread;
    }

    this->split_->setInputReply(thread);
}

void ChannelView::showReplyThreadPopup(const MessagePtr &message)
{
    if (message == nullptr || message->replyThread == nullptr)
    {
        return;
    }

    auto popupParent =
        static_cast<QWidget *>(&(getApp()->windows->getMainWindow()));
    auto popup = new ReplyThreadPopup(getSettings()->autoCloseThreadPopup,
                                      popupParent, this->split_);

    popup->setThread(message->replyThread);

    QPoint offset(int(150 * this->scale()), int(70 * this->scale()));
    popup->move(QCursor::pos() - offset);
    popup->show();
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

    if (this->channel_ == nullptr)
    {
        return false;
    }

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

}  // namespace chatterino
