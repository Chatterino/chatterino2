#include "MessageView.hpp"

#include "Application.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "messages/Selection.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"

#include <QApplication>
#include <QPainter>

namespace chatterino {

namespace {
    Selection emptySelection;
}

MessageView::MessageView()
    : MessageView(nullptr)
{
}

MessageView::MessageView(MessagePtr message)
    : message_(std::move(message))
    , width_(0)
{
    this->createMessageLayout();

    // Configure theme and preferences for rendering message
    this->messageColors_.applyTheme(getTheme());
    this->messagePreferences_.connectSettings(getSettings(),
                                              this->signalHolder_);

    // Update frame for any GIFs
    this->signalHolder_.managedConnect(
        getIApp()->getWindows()->gifRepaintRequested, [&] {
            this->maybeUpdate();
        });

    // Re-layout and potentially update if base flags change (e.g. settings for badges).
    this->signalHolder_.managedConnect(getApp()->windows->wordFlagsChanged,
                                       [this] {
                                           this->layoutMessage();
                                       });
}

MessageView::~MessageView()
{
}

void MessageView::createMessageLayout()
{
    if (this->message_ == nullptr)
    {
        this->messageLayout_.reset();
        return;
    }

    this->messageLayout_ = std::make_unique<MessageLayout>(this->message_);
}

void MessageView::setMessage(MessagePtr message)
{
    if (this->message_ != message)
    {
        this->message_ = std::move(message);
        this->createMessageLayout();
        this->layoutMessage();
    }
}

void MessageView::clearMessage()
{
    this->setMessage(nullptr);
}

void MessageView::setWidth(int width)
{
    if (this->width_ != width)
    {
        this->width_ = width;
        this->layoutMessage();
    }
}

void MessageView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    auto ctx = MessagePaintContext{
        .painter = painter,
        .selection = emptySelection,
        .colorProvider = ColorProvider::instance(),
        .messageColors = this->messageColors_,
        .preferences = this->messagePreferences_,

        .canvasWidth = this->width_,
        .isWindowFocused = this->window() == QApplication::activeWindow(),
        .isMentions = false,

        .y = 0,
        .messageIndex = 0,
        .isLastReadMessage = false,
    };

    this->messageLayout_->paint(ctx);
}

void MessageView::themeChangedEvent()
{
    this->layoutMessage();
}

void MessageView::scaleChangedEvent(float /*newScale*/)
{
    this->layoutMessage();
}

void MessageView::maybeUpdate()
{
    if (this->messageLayout_ != nullptr)
    {
        this->update();
    }
}

void MessageView::layoutMessage()
{
    if (this->messageLayout_ == nullptr)
    {
        return;
    }

    auto flags = getFlags();
    bool updateRequired =
        this->messageLayout_->layout(this->width_, this->scale(), flags);

    if (updateRequired)
    {
        this->setFixedSize(this->width_, this->messageLayout_->getHeight());
        this->update();
    }
}

MessageElementFlags MessageView::getFlags() const
{
    // Start with base global flags
    auto flags = getIApp()->getWindows()->getWordFlags();

    // Don't show inline replies or reply button
    flags.unset(MessageElementFlag::RepliedMessage);
    flags.unset(MessageElementFlag::ReplyButton);

    return flags;
}

}  // namespace chatterino
