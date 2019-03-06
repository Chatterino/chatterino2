#include "ChannelView.hpp"

#include "Application.hpp"
#include "Window.hpp"
#include "ab/Column.hpp"
#include "ab/FlatButton.hpp"
#include "ab/Row.hpp"
#include "ab/util/Benchmark.hpp"
#include "ab/util/FunctionEventFilter.hpp"
#include "ab/util/MakeWidget.hpp"
#include "messages/Common.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "ui/Scrollbar.hpp"
#include "ui/Tooltip.hpp"
#include "util/DistanceBetweenPoints.hpp"
#include "util/IncognitoBrowser.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QShortcut>

#include <chrono>
#include <functional>
#include <memory>

#define DRAW_WIDTH (this->width())
#define SELECTION_RESUME_SCROLLING_MSG_THRESHOLD 3
#define CHAT_HOVER_PAUSE_DURATION 1000

namespace chatterino::ui
{
    namespace
    {
        void addEmoteContextMenuItems(
            const Emote& emote, MessageElementFlags creatorFlags, QMenu& menu)
        {
            auto openAction = menu.addAction("Open");
            auto openMenu = new QMenu;
            openAction->setMenu(openMenu);

            auto copyAction = menu.addAction("Copy");
            auto copyMenu = new QMenu;
            copyAction->setMenu(copyMenu);

            // Add copy and open links for 1x, 2x, 3x
            auto addImageLink = [&](const ImagePtr& image, char scale) {
                if (!image->isEmpty())
                {
                    copyMenu->addAction(
                        QString(scale) + "x link", [url = image->url()] {
                            QApplication::clipboard()->setText(url.string);
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
            auto addPageLink = [&](const QString& name) {
                copyMenu->addSeparator();
                openMenu->addSeparator();

                copyMenu->addAction(
                    "Copy " + name + " emote link", [url = emote.homePage] {
                        QApplication::clipboard()->setText(url.string);  //
                    });
                openMenu->addAction(
                    "Open " + name + " emote link", [url = emote.homePage] {
                        QDesktopServices::openUrl(QUrl(url.string));  //
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
        }

        // TODO: dependency inject
        ThemexD& getTheme(ChannelView& self)
        {
            if (auto window = dynamic_cast<Window*>(self.window()))
            {
                return window->theme();
            }
            else
            {
                static QWidget widget;
                static ThemexD theme(&widget);

                return theme;
            }
        }
    }  // namespace

    ChannelView::ChannelView(QWidget* parent)
        : BaseWidget(parent)
    {
        this->setMouseTracking(true);

        this->initializeLayout();
        this->initializeSignals();

        auto shortcut = new QShortcut(QKeySequence("Ctrl+C"), this);
        QObject::connect(shortcut, &QShortcut::activated, [this] {
            QGuiApplication::clipboard()->setText(this->selectedText());
        });

        this->clickTimer_ = new QTimer(this);
        this->clickTimer_->setSingleShot(true);
        this->clickTimer_->setInterval(300);
        QObject::connect(this->clickTimer_, &QTimer::timeout, this,
            [this]() { this->nextClickIsTriple = false; });

        // TODO: dependency inject
        this->installEventFilter(new ab::FunctionEventFilter(
            this, [this](QObject*, QEvent* event) -> bool {
                if (event->type() == QEvent::ParentChange)
                {
                    this->theme = &getTheme(*this);

                    this->queueLayout();
                }
                else if (event->type() == QEvent::Show)
                {
                    this->theme = &getTheme(*this);

                    this->queueLayout();
                }
                return false;
            }));

        // unpaused
        QObject::connect(&this->pauser, &Pauser::unpaused, this, [this]() {
            this->scrollBar_->offset(this->pauseScrollOffset_);
            this->pauseScrollOffset_ = 0;

            this->queueLayout();
        });
    }

    void ChannelView::initializeLayout()
    {
        // set layout
        this->setLayout(ab::makeLayout<ab::Row>({
            ab::stretch(),
            ab::makeLayout<ab::Column>({
                ab::stretch(),
                ab::makeWidget<ab::FlatButton>([&](auto w) {
                    auto label = new QLabel("More messages below");
                    label->setAlignment(Qt::AlignCenter);
                    w->setObjectName("scroll-to-bottom");

                    this->goToBottom_ = w;
                    w->setChild(label);
                    QObject::connect(w, &ab::FlatButton::leftClicked, this, [] {
                        QTimer::singleShot(180, [=] {
                            // this->scrollBar_->scrollToBottom(
                            //    getSettings()
                            // ->enableSmoothScrollingNewMessages
                            //        .getValue());
                        });
                    });
                }),
            }),
        }));

        // FOURTF: for some reason this doesn't layout properly in a Row
        ab::makeWidget<Scrollbar>([&](Scrollbar* w) {
            this->scrollBar_ = w;
            w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

            QObject::connect(this->scrollBar_, &Scrollbar::currentValueChanged,
                this, [this] {
                    this->performLayout(true);
                    this->queueUpdate();
                });
            //     this->goToBottom_, &EffectLabel::leftClicked, this,
            //     [=] {
            //         QTimer::singleShot(180, [=] {
            //             this->scrollBar_->scrollToBottom(
            //                 getSettings()
            //                     ->enableSmoothScrollingNewMessages.getValue());
            //         });
            //     });
        });
        this->scrollBar_->setParent(this);
    }

    void ChannelView::initializeSignals()
    {
        // this->connections_.push_back(
        //    getApp()->windows->wordFlagsChanged.connect([this] {
        //        this->queueLayout();
        //        this->update();
        //    }));

        // getSettings()->showLastMessageIndicator.connect(
        //    [this](auto, auto) { this->update(); }, this->connections_);

        // connections_.push_back(getApp()->windows->gifRepaintRequested.connect(
        //    [&] { this->queueUpdate(); }));

        // connections_.push_back(
        //    getApp()->windows->layoutRequested.connect([&](Channel* channel) {
        //        if (this->isVisible() &&
        //            (channel == nullptr || this->channel_.get() == channel))
        //        {
        //            this->queueLayout();
        //        }
        //    }));

        // connections_.push_back(getApp()->fonts->fontChanged.connect(
        //    [this] { this->queueLayout(); }));

        QObject::connect(&imageUpdated(), &ImageUpdated::imageUpdated, this,
            [this]() { this->queueLayout(); });
    }

    void ChannelView::messagesInserted(
        const MessageContainer::Insertion& change)
    {
        assert(this->layouts_.size() <= change.index);

        // create vector for layouts
        std::vector<std::unique_ptr<MessageLayout>> elements;
        elements.reserve(change.messages.size());

        // convert messages into layout
        for (auto&& message : change.messages)
            elements.push_back(std::make_unique<MessageLayout>(message));

        // insert layouts
        this->layouts_.insert(this->layouts_.begin() + change.index,
            std::make_move_iterator(elements.begin()),
            std::make_move_iterator(elements.end()));

        // queue stuff
        this->queueLayout();
    }

    void ChannelView::messagesErased(const MessageContainer::Erasure& erasure)
    {
        assert(this->layouts_.size() < erasure.index);

        // erase items from list of messages on screen
        for (size_t i = erasure.index; i < erasure.count; i++)
        {
            auto it = this->messagesOnScreen_.find(this->layouts_[i].get());
            if (it != this->messagesOnScreen_.end())
                this->messagesOnScreen_.erase(it);
        }

        // erase items
        this->layouts_.erase(this->layouts_.begin() + erasure.index,
            this->layouts_.begin() + erasure.index + erasure.count);

        // queue stuff
        this->queueLayout();
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
        auto&& layouts = this->layouts_;

        this->showingLatestMessages_ =
            this->scrollBar_->isAtBottom() || !this->scrollBar_->isVisible();

        /// Layout visible messages
        this->layoutVisibleMessages(layouts);

        /// Update scrollbar
        this->updateScrollbar(layouts, causedByScrollbar);

        if (this->goToBottom_)
            this->goToBottom_->setVisible(this->enableScrollingToBottom_ &&
                                          this->scrollBar_->isVisible() &&
                                          !this->scrollBar_->isAtBottom());
    }

    void ChannelView::layoutVisibleMessages(const LayoutContainerType& messages)
    {
        const auto start = size_t(this->scrollBar_->getCurrentValue());
        const auto layoutWidth = this->layoutWidth();
        const auto flags = this->flags();
        auto redrawRequired = false;

        if (messages.size() > start)
        {
            auto y = int(-(messages[start].get()->getHeight() *
                           (fmod(this->scrollBar_->getCurrentValue(), 1))));

            for (auto i = start; i < messages.size() && y <= this->height();
                 i++)
            {
                auto&& message = messages[i];

                redrawRequired |= message->layout(
                    *this->theme, layoutWidth, this->scale(), flags);

                y += message->getHeight();
            }
        }

        if (redrawRequired)
            this->queueUpdate();
    }

    void ChannelView::updateScrollbar(
        const LayoutContainerType& messages, bool causedByScrollbar)
    {
        if (messages.size() == 0)
        {
            this->scrollBar_->setVisible(false);
            return;
        }

        /// Layout the messages at the bottom
        auto h = this->height() - 8;
        auto flags = this->flags();
        auto layoutWidth = this->layoutWidth();
        auto showScrollbar = false;

        // convert i to int since it checks >= 0
        for (auto i = int(messages.size()) - 1; i >= 0; i--)
        {
            auto* message = messages[i].get();

            message->layout(*this->theme, layoutWidth, this->scale(), flags);

            h -= message->getHeight();

            if (h < 0)  // break condition
            {
                this->scrollBar_->setLargeChange(
                    (messages.size() - i) + qreal(h) / message->getHeight());

                showScrollbar = true;
                break;
            }
        }

        /// Update scrollbar values
        this->scrollBar_->setVisible(showScrollbar);
        // TODO: remove
        this->scrollBar_->setGeometry(this->width() - this->scrollBar_->width(),
            0, this->scrollBar_->width(), this->height());

        if (!showScrollbar && !causedByScrollbar)
        {
            this->scrollBar_->setDesiredValue(0);
        }

        this->scrollBar_->setMaximum(messages.size());

        // If we were showing the latest messages and the scrollbar now wants to
        // be rendered, scroll to bottom
        if (this->enableScrollingToBottom_ && this->showingLatestMessages_ &&
            showScrollbar)
        {
            this->scrollBar_->scrollToBottom(
                // this->messageWasAdded &&
                // getSettings()->enableSmoothScrollingNewMessages.getValue()
                true);
            this->messageWasAdded_ = false;
        }
    }

    void ChannelView::clearMessages()
    {
        // TODO: implement

        //// Clear all stored messages in this chat widget
        //// this->messages_.clear();
        // this->scrollBar_->clearHighlights();
        // this->queueLayout();
    }

    Scrollbar& ChannelView::scrollbar()
    {
        return *this->scrollBar_;
    }

    QString ChannelView::selectedText()
    {
        QString result = "";

        const auto& items = this->layouts_;
        const auto& selection = this->selector_.selection();

        if (selection.isEmpty())
            return result;

        for (int msg = selection.start.messageIndex;
             msg <= selection.end.messageIndex; msg++)
        {
            const auto& layout = items[msg];

            int from = msg == selection.start.messageIndex
                           ? selection.start.charIndex
                           : 0;
            int to = msg == selection.end.messageIndex
                         ? selection.end.charIndex
                         : layout->getLastCharacterIndex() + 1;

            layout->addSelectionText(result, from, to);
        }

        return result;
    }

    bool ChannelView::hasSelection()
    {
        return !this->selector_.selection().isEmpty();
    }

    void ChannelView::clearSelection()
    {
        this->selector_.clear();

        queueLayout();
    }

    void ChannelView::setEnableScrollingToBottom(bool value)
    {
        this->enableScrollingToBottom_ = value;
    }

    bool ChannelView::enableScrollingToBottom() const
    {
        return this->enableScrollingToBottom_;
    }

    void ChannelView::setOverrideFlags(std::optional<MessageElementFlags> value)
    {
        this->overrideFlags_ = value;
    }

    const std::optional<MessageElementFlags>& ChannelView::overrideFlags() const
    {
        return this->overrideFlags_;
    }

    bool ChannelView::pausable() const
    {
        return this->pauser.pausable();
    }

    void ChannelView::setPausable(bool val)
    {
        this->pauser.setPausable(val);
    }

    std::shared_ptr<MessageContainer> ChannelView::container()
    {
        return this->messages_;
    }

    void ChannelView::setContainer(
        const std::shared_ptr<MessageContainer>& newChannel)
    {
        // Clear connections from the last channel
        for (auto&& connection : this->channelConnections_)
            QObject::disconnect(connection);

        this->channelConnections_.clear();

        // clear messages
        this->clearMessages();

        // on message inserted
        this->channelConnections_.push_back(QObject::connect(newChannel.get(),
            &MessageContainer::inserted, this, &ChannelView::messagesInserted));

        // on message erased
        this->channelConnections_.push_back(QObject::connect(newChannel.get(),
            &MessageContainer::erased, this, &ChannelView::messagesErased));

        // TODO: add all current messages
        // auto snapshot = newChannel->getMessageSnapshot();

        // for (size_t i = 0; i < snapshot.size(); i++)
        //{
        //    MessageLayoutPtr deleted;

        //    auto messageRef = new MessageLayout(snapshot[i]);

        //    if (this->lastMessageHasAlternateBackground_)
        //    {
        //        messageRef->flags.set(MessageLayoutFlag::AlternateBackground);
        //    }
        //    this->lastMessageHasAlternateBackground_ =
        //        !this->lastMessageHasAlternateBackground_;

        //    this->messages_.pushBack(MessageLayoutPtr(messageRef), deleted);
        //}

        this->messages_ = newChannel;

        this->queueLayout();
        this->queueUpdate();
    }

    void ChannelView::updateLastReadMessage()
    {
        const auto& items = this->layouts_;

        if (!items.empty())
        {
            this->lastReadMessage_ = items.back().get();

            this->update();
        }
    }

    //    void ChannelView::setSelection(
    //        const SelectionItem& start, const SelectionItem& end)
    //    {
    //        // selections
    //        if (!this->selecting_ && start != end)
    //        {
    //            // this->messagesAddedSinceSelectionPause_ = 0;

    //            this->selecting_ = true;
    //            // this->pausedBySelection_ = true;
    //        }

    //        this->selection_ = Selection(start, end);

    //        emit this->selectionChanged();
    //    }

    MessageElementFlags ChannelView::flags() const
    {
        if (this->overrideFlags_)
        {
            return this->overrideFlags_.value();
        }
        // TODO: replace with better code
        // else if (auto split = dynamic_cast<Split*>(this->parentWidget()))
        //{
        // auto flags = app->windows->getWordFlags();
        //    // TODO: ???
        //    // if (split->getModerationMode())
        //    // {
        //    //     flags.set(MessageElementFlag::ModeratorTools);
        //    // }
        //    // if (this->channel_ == app->twitch.server->mentionsChannel)
        //    //{
        //    //    flags.set(MessageElementFlag::ChannelName);
        //    //}
        // return flags;
        //}
        else
        {
            return {};  // = app->windows->getWordFlags();
        }
    }

    void ChannelView::resizeEvent(QResizeEvent*)
    {
        this->queueLayout();

        // FOURTF: for some reason this doesn't layout properly in a Row
        this->scrollBar_->setGeometry(this->width() - this->scrollBar_->width(),
            0, this->scrollBar_->width(), this->height());
    }

    void ChannelView::paintEvent(QPaintEvent* /*event*/)
    {
        //    BenchmarkGuard benchmark("paint");

        QPainter painter(this);

        painter.fillRect(rect(), this->palette().background());

        // draw messages
        this->drawMessages(painter);

        // draw paused sign
        if (this->pauser.paused())
        {
            auto a = this->scale() * 16;
            auto brush = QBrush(QColor(127, 127, 127, 63));
            painter.fillRect(QRectF(this->width() - a, a / 4, a / 4, a), brush);
            painter.fillRect(
                QRectF(this->width() - a / 2, a / 4, a / 4, a), brush);
        }
    }

    // if overlays is false then it draws the message, if true then it draws
    // things such as the grey overlay when a message is disabled
    void ChannelView::drawMessages(QPainter& painter)
    {
        auto&& layouts = this->layouts_;

        size_t start = size_t(this->scrollBar_->getCurrentValue());

        if (start >= layouts.size())
        {
            return;
        }

        int y = int(-(layouts[start].get()->getHeight() *
                      (fmod(this->scrollBar_->getCurrentValue(), 1))));

        MessageLayout* end = nullptr;
        bool windowFocused = this->window() == QApplication::activeWindow();

        for (size_t i = start; i < layouts.size(); ++i)
        {
            MessageLayout* layout = layouts[i].get();

            bool isLastMessage = false;
            // if (getSettings()->showLastMessageIndicator)
            //{
            //    isLastMessage = this->lastReadMessage_.get() == layout;
            //}

            layout->paint(*this->theme, painter, DRAW_WIDTH, y, i,
                this->selector_.selection(), isLastMessage, windowFocused);

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
        for (size_t i = start; i < layouts.size(); ++i)
        {
            auto it = this->messagesOnScreen_.find(layouts[i].get());
            if (it != this->messagesOnScreen_.end())
            {
                this->messagesOnScreen_.erase(it);
            }
        }

        // delete the message buffers that aren't on screen
        for (auto&& item : this->messagesOnScreen_)
        {
            item->deleteBuffer();
        }

        this->messagesOnScreen_.clear();

        // add all messages on screen to the map
        for (size_t i = start; i < layouts.size(); ++i)
        {
            auto&& layout = layouts[i];

            this->messagesOnScreen_.insert(layout.get());

            if (layout.get() == end)
            {
                break;
            }
        }
    }

    void ChannelView::wheelEvent(QWheelEvent* event)
    {
        if (event->orientation() != Qt::Vertical)
            return;

        if (event->modifiers() & Qt::ControlModifier)
        {
            event->ignore();
            return;
        }

        if (this->scrollBar_->isVisible())
        {
            float mouseMultiplier = 1;  // getSettings()->mouseScrollMultiplier;
            qreal desired = this->scrollBar_->getDesiredValue();
            qreal delta = event->delta() * qreal(1.5) * mouseMultiplier;

            const auto& items = this->layouts_;

            // signed because of comparisons
            auto itemCount = signed(items.size());
            auto i = std::min<signed>(signed(desired), itemCount);

            if (delta > 0)
            {
                qreal scrollFactor = fmod(desired, 1);
                qreal currentScrollLeft =
                    int(scrollFactor * items[i]->getHeight());

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
                        items[i - 1]->layout(*this->theme, this->layoutWidth(),
                            this->scale(), this->flags());
                        scrollFactor = 1;
                        currentScrollLeft = items[i - 1]->getHeight();
                    }
                }
            }
            else
            {
                delta = -delta;
                qreal scrollFactor = 1 - fmod(desired, 1);
                qreal currentScrollLeft =
                    int(scrollFactor * items[i]->getHeight());

                for (; i < itemCount; i++)
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

                    if (i == itemCount - 1)
                    {
                        desired = itemCount;
                    }
                    else
                    {
                        items[i + 1]->layout(*this->theme, this->layoutWidth(),
                            this->scale(), this->flags());

                        scrollFactor = 1;
                        currentScrollLeft = items[i + 1]->getHeight();
                    }
                }
            }

            this->scrollBar_->setDesiredValue(desired, true);
        }
    }

    void ChannelView::enterEvent(QEvent*)
    {
    }

    void ChannelView::leaveEvent(QEvent*)
    {
        this->pauser.unpause(PauseReason::Mouse);

        this->queueLayout();
    }

    void ChannelView::mouseMoveEvent(QMouseEvent* event)
    {
        if (event->modifiers() & (Qt::AltModifier | Qt::ControlModifier))
        {
            this->unsetCursor();

            event->ignore();
            return;
        }

        /// Pause on hover
        if (true)  // getSettings()->pauseChatOnHover.getValue())
        {
            this->pauser.pause(PauseReason::Mouse, 500);
        }

        auto tooltipWidget = Tooltip::instance();

        auto msg = messageAt(event->pos());

        // no message under cursor
        if (!msg)
        {
            this->setCursor(Qt::ArrowCursor);
            tooltipWidget->hide();
            return;
        }

        auto& [layout, relativePos, messageIndex] = msg.value();

        // is selecting
        if (this->isMouseDown_ && !this->isDoubleClick_)
        {
            // this->pause(PauseReason::Selecting, 300);

            this->selector_.moveRegular(
                {messageIndex, layout.getSelectionIndex(relativePos)});

            this->queueUpdate();
        }

        // message under cursor is collapsed
        if (layout.flags.has(MessageLayoutFlag::Collapsed))
        {
            this->setCursor(Qt::PointingHandCursor);
            tooltipWidget->hide();
            return;
        }

        // check if word underneath cursor
        auto* hoverLayoutElement = layout.getElementAt(relativePos);

        if (hoverLayoutElement == nullptr)
        {
            this->setCursor(Qt::ArrowCursor);
            tooltipWidget->hide();
            return;
        }

        if (this->isMouseDown_ && this->isDoubleClick_)
        {
            auto [wordStart, wordEnd] =
                this->wordBounds(&layout, hoverLayoutElement, relativePos);

            this->selector_.moveWord(
                {messageIndex, wordStart}, {messageIndex, wordEnd});
            this->queueUpdate();
        }

        // tooltipWidget->setText(
        //    "memesasdf asdasdf asdf" + QString(rand() % 20, '\n') + "asdf");
        // tooltipWidget->moveTo(this, event->globalPos());
        // tooltipWidget->show();
        // tooltipWidget->raise();
        // QTimer::singleShot(100, this, [=]() { tooltipWidget->adjustSize();
        // });

        return;
        // TODO: this crashes
        auto tooltip = hoverLayoutElement->getCreator().getTooltip();
        bool isLinkValid = hoverLayoutElement->getLink().isValid();

        if (tooltip.isEmpty())
        {
            tooltipWidget->hide();
        }
        else if (isLinkValid)  // TODO: && !getSettings()->linkInfoTooltip)
        {
            tooltipWidget->hide();
        }
        else
        {
            tooltipWidget->moveTo(this, event->globalPos());
            tooltipWidget->setWordWrap(isLinkValid);
            tooltipWidget->setText(tooltip);
            tooltipWidget->show();
            tooltipWidget->adjustSize();
            tooltipWidget->raise();
        }

        if (isLinkValid)
            this->setCursor(Qt::PointingHandCursor);
        else
            this->setCursor(Qt::ArrowCursor);
    }

    void ChannelView::mousePressEvent(QMouseEvent* event)
    {
        ab::BaseWidget::mousePressEvent(event);

        emit mouseDown(event);

        if (auto msg = messageAt(event->pos()))
            this->messagePressed(msg.value(), event);
        else
            this->pressedBelowMessages(event);
    }

    void ChannelView::messagePressed(MessageAt& msg, QMouseEvent* event)
    {
        auto& [layout, relativePos, messageIndex] = msg;

        // check if message is collapsed
        if (event->button() == Qt::LeftButton)
        {
            this->lastPressPosition_ = event->screenPos();
            this->isMouseDown_ = true;

            if (layout.flags.has(MessageLayoutFlag::Collapsed))
                return;

            // TODO: reenable
            // if (getSettings()->linksDoubleClickOnly.getValue())
            //{
            //    this->pauser.pause(PauseReason::DoubleClick, 200);
            //}

            // start selection
            if (auto* element = layout.getElementAt(relativePos))
            {
                if (this->nextClickIsTriple)
                {
                    this->selectWholeMessage(layout, messageIndex);
                }
                else
                {
                    auto [wordStart, wordEnd] =
                        this->wordBounds(&layout, element, relativePos);

                    this->selector_.start(
                        {messageIndex, layout.getSelectionIndex(relativePos)},
                        {messageIndex, wordStart}, {messageIndex, wordEnd});
                }
            }
        }
        else if (event->button() == Qt::RightButton)
        {
            this->lastRightPressPosition_ = event->screenPos();
            this->isRightMouseDown_ = true;
        }

        this->update();
    }

    void ChannelView::pressedBelowMessages(QMouseEvent* event)
    {
        this->setCursor(Qt::ArrowCursor);

        if (const auto& layouts = this->layouts_; !layouts.empty())
        {
            // Start selection at the last message at its last index
            if (event->button() == Qt::LeftButton)
            {
                auto selectionItem = SelectionItem{
                    int(layouts.size() - 1),
                    layouts.back()->getLastCharacterIndex(),
                };

                // TODO: might be broken but maybe it's not
                this->selector_.start(
                    selectionItem, selectionItem, selectionItem);
            }
        }
    }

    void ChannelView::mouseReleaseEvent(QMouseEvent* event)
    {
        ab::BaseWidget::mouseReleaseEvent(event);

        // TODO: scale 15
        if (distance(this->lastPressPosition_, event->screenPos()) > 15)
        {
            this->clickTimer_->stop();

            this->isDoubleClick_ = false;
            this->nextClickIsTriple = false;
            this->isMouseDown_ = false;
            this->isRightMouseDown_ = false;
        }
        else
        {
            if (event->button() == Qt::LeftButton)
            {
                this->isMouseDown_ = false;

                if (auto msg = messageAt(event->pos()))
                    this->messageClicked(msg.value(), event);
            }
            else if (event->button() == Qt::RightButton)
            {
                this->isRightMouseDown_ = false;
            }
        }

        this->queueLayout();
    }

    void ChannelView::messageClicked(MessageAt& msg, QMouseEvent* event)
    {
        auto& [layout, relativePos, index] = msg;

        // message under cursor is collapsed
        if (layout.flags.has(MessageLayoutFlag::Collapsed))
        {
            layout.flags.set(MessageLayoutFlag::Expanded);
            layout.flags.set(MessageLayoutFlag::RequiresLayout);

            this->queueLayout();
        }
        else
        {
            // Triple-clicking a message selects the whole message
            if (isDoubleClick_ /*&& this->selecting_*/)
                this->selectWholeMessage(layout, index);

            // handle the click
            if (auto* hoverLayoutElement = layout.getElementAt(relativePos))
                this->handleMouseClick(event, hoverLayoutElement, &layout);
        }
    }

    void ChannelView::handleMouseClick(QMouseEvent* event,
        const MessageLayoutElement* hoveredElement, MessageLayout* layout)
    {
        if (event->button() == Qt::LeftButton)
        {
            // if (this->selecting_)
            // {
            //     // this->pausedBySelection = false;
            //     this->selecting_ = false;
            //     // this->pauseTimeout.stop();
            //     // this->pausedTemporarily = false;

            //     this->queueLayout();
            // }

            // auto& link = hoveredElement->getLink();

            // TODO:
            // if (!getSettings()->linksDoubleClickOnly)
            //{
            //    this->handleLinkClick(event, link, layout);
            //}

            // Invoke to signal from EmotePopup.
            // if (link.type == Link::InsertText)
            //{
            //    this->linkClicked.invoke(link);
            //}
        }
        else if (event->button() == Qt::RightButton)
        {
            // TODO: fix
            // auto insertText = [=](QString text) {
            //    if (auto split = dynamic_cast<Split*>(this->parentWidget()))
            //    {
            //        // split->insertTextToInput(text);
            //    }
            //};

            // auto& link = hoveredElement->getLink();

            // if (link.type == Link::UserInfo)
            //    insertText("@" + link.value + ", ");
            // else if (link.type == Link::UserWhisper)
            //    insertText("/w " + link.value + " ");
            // else
            //    this->addContextMenuItems(hoveredElement, layout);
        }
    }

    // TODO: move to provider
    void ChannelView::addContextMenuItems(
        const MessageLayoutElement* hoveredElement, MessageLayout* layout)
    {
        const auto& creator = hoveredElement->getCreator();
        auto creatorFlags = creator.getFlags();

        static QMenu* menu = new QMenu;
        menu->clear();

        // Emote actions
        if (creatorFlags.hasAny({MessageElementFlag::EmoteImages,
                MessageElementFlag::EmojiImage}))
        {
            const auto emoteElement =
                dynamic_cast<const EmoteElement*>(&creator);
            if (emoteElement)
                addEmoteContextMenuItems(
                    *emoteElement->getEmote(), creatorFlags, *menu);
        }

        // add seperator
        if (!menu->actions().empty())
        {
            menu->addSeparator();
        }

        // Link copy
        if (hoveredElement->getLink().type == Link::Url)
        {
            QString url = hoveredElement->getLink().value;

            // open link
            bool incognitoByDefault =
                supportsIncognitoLinks() &&
                layout->getMessage()->loginName == "hemirt";
            menu->addAction("Open link", [url, incognitoByDefault] {
                if (incognitoByDefault)
                    openLinkIncognito(url);
                else
                    QDesktopServices::openUrl(QUrl(url));
            });
            // open link default
            if (supportsIncognitoLinks())
            {
                menu->addAction(
                    "Open link incognito", [url] { openLinkIncognito(url); });
            }
            menu->addAction("Copy link",
                [url] { QApplication::clipboard()->setText(url); });

            menu->addSeparator();
        }

        // Copy actions
        if (!this->selector_.selection().isEmpty())
        {
            menu->addAction("Copy selection", [this] {
                QGuiApplication::clipboard()->setText(this->selectedText());
            });
        }

        menu->addAction("Copy message", [layout] {
            QString copyString;
            layout->addSelectionText(
                copyString, 0, INT_MAX, CopyMode::OnlyTextAndEmotes);

            QGuiApplication::clipboard()->setText(copyString);
        });

        menu->addAction("Copy full message", [layout] {
            QString copyString;
            layout->addSelectionText(copyString);

            QGuiApplication::clipboard()->setText(copyString);
        });

        // Open in new split.
        if (hoveredElement->getLink().type == Link::Url)
        {
            static QRegularExpression twitchChannelRegex(
                R"(^(?:https?:\/\/)?(?:www\.|go\.)?twitch\.tv\/(?<username>[a-z0-9_]{3,}))",
                QRegularExpression::CaseInsensitiveOption);
            static QSet<QString> ignoredUsernames{
                "videos",
                "settings",
                "directory",
                "jobs",
                "friends",
                "inventory",
                "payments",
                "subscriptions",
                "messages",
            };

            auto twitchMatch =
                twitchChannelRegex.match(hoveredElement->getLink().value);
            auto twitchUsername = twitchMatch.captured("username");
            if (!twitchUsername.isEmpty() &&
                !ignoredUsernames.contains(twitchUsername))
            {
                menu->addSeparator();
                menu->addAction("Open in new split", [twitchUsername, this] {
                    // TODO: implement
                    // this->joinToChannel.invoke(twitchUsername);
                });
            }
        }

        menu->popup(QCursor::pos());
        menu->raise();

        return;
    }

    void ChannelView::mouseDoubleClickEvent(QMouseEvent* event)
    {
        ab::BaseWidget::mouseDoubleClickEvent(event);

        if (auto msg = messageAt(event->pos()))
            messageDoubleClicked(msg.value(), event);
    }

    void ChannelView::messageDoubleClicked(MessageAt& msg, QMouseEvent* event)
    {
        auto& [layout, relativePos, index] = msg;

        // message under cursor is collapsed
        if (layout.flags.has(MessageLayoutFlag::Collapsed))
            return;

        // double click
        this->clickTimer_->start();
        this->isDoubleClick_ = true;
        this->nextClickIsTriple = true;

        if (auto* element = layout.getElementAt(relativePos))
        {
            auto [wordStart, wordEnd] =
                this->wordBounds(&layout, element, relativePos);

            this->selector_.moveWord({index, wordStart}, {index, wordEnd});
            this->queueUpdate();

            // TODO: implement
            // if (getSettings()->linksDoubleClickOnly)
            //{
            //    auto& link = element->getLink();
            //    this->handleLinkClick(event, link, layout.get());
            //}
        }
    }

    void ChannelView::hideEvent(QHideEvent*)
    {
        for (auto&& layout : this->messagesOnScreen_)
            layout->deleteBuffer();

        this->messagesOnScreen_.clear();
    }

    // TODO: move to provider
    void ChannelView::showUserInfoPopup(const QString& userName)
    {
        // TODO: move to proper place
        // auto* userPopup = new UserInfoPopup;
        // userPopup->setData(userName, this->channel_);
        // userPopup->setActionOnFocusLoss(BaseWindow::Delete);
        // QPoint offset(int(150 * this->scale()), int(70 *
        // this->scale())); userPopup->move(QCursor::pos() -
        // offset); userPopup->show();
    }

    // TODO: move to provider
    void ChannelView::handleLinkClick(
        QMouseEvent* event, const Link& link, MessageLayout* layout)
    {
        if (event->button() != Qt::LeftButton)
            return;

        switch (link.type)
        {
            case Link::UserWhisper:
            case Link::UserInfo:
            {
                auto user = link.value;
                this->showUserInfoPopup(user);
                qDebug() << "Clicked " << user << "s message";
            }
            break;

            case Link::Url:
            {
                QDesktopServices::openUrl(QUrl(link.value));
            }
            break;

            case Link::UserAction:
            {
                QString value = link.value;
                value.replace("{user}", layout->getMessage()->loginName);
                // TODO: implement
                // this->channel_->sendMessage(value);
            }
            break;

            default:;
        }
    }

    std::optional<ChannelView::MessageAt> ChannelView::messageAt(QPoint p)
    {
        const auto& items = this->layouts_;
        const auto start = size_t(this->scrollBar_->getCurrentValue());

        if (start >= items.size())
            return {};

        auto y = int(-(items[start]->getHeight() *
                       (fmod(this->scrollBar_->getCurrentValue(), 1))));

        for (auto i = start; i < items.size() && y < this->height(); ++i)
        {
            auto&& message = items[i];

            if (p.y() < y + message->getHeight())
                return {{*message, QPoint(p.x(), p.y() - y), int(i)}};

            y += message->getHeight();
        }

        return {};
    }

    int ChannelView::layoutWidth() const
    {
        if (this->scrollBar_->isVisible())
            return int(this->width() - scrollbarPadding * this->scale());
        else
            return this->width();
    }

    void ChannelView::selectWholeMessage(
        MessageLayout& layout, int& messageIndex)
    {
        SelectionItem msgStart(
            messageIndex, layout.getFirstMessageCharacterIndex());
        SelectionItem msgEnd(messageIndex, layout.getLastCharacterIndex());

        this->selector_.start(msgStart, msgStart, msgStart);
        this->selector_.moveRegular(msgEnd);
    }

    std::pair<int, int> ChannelView::wordBounds(MessageLayout* layout,
        const MessageLayoutElement* element, const QPoint& relativePos)
    {
        const int mouseInWordIndex = element->getMouseOverIndex(relativePos);
        const int selectionLength = element->getSelectionIndexCount();
        const int length =
            element->hasTrailingSpace() ? selectionLength - 1 : selectionLength;
        const int wordStart =
            layout->getSelectionIndex(relativePos) - mouseInWordIndex;

        return {wordStart, wordStart + length};
    }
}  // namespace chatterino::ui
