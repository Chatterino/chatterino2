#include "widgets/helper/MessageView.hpp"

#include "Application.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "messages/MessageElement.hpp"
#include "messages/Selection.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"

#include <QApplication>
#include <QPainter>

namespace {

using namespace chatterino;

const Selection EMPTY_SELECTION;

const MessageElementFlags MESSAGE_FLAGS{
    MessageElementFlag::Text,
    MessageElementFlag::EmojiAll,
    MessageElementFlag::EmoteText,
};

}  // namespace

namespace chatterino {

MessageView::MessageView() = default;
MessageView::~MessageView() = default;

void MessageView::createMessageLayout()
{
    if (this->message_ == nullptr)
    {
        this->messageLayout_.reset();
        return;
    }

    this->messageLayout_ = std::make_unique<MessageLayout>(this->message_);
}

void MessageView::setMessage(const MessagePtr &message)
{
    if (!message)
    {
        return;
    }

    auto singleLineMessage = std::make_shared<Message>();
    singleLineMessage->elements.emplace_back(
        std::make_unique<SingleLineTextElement>(
            message->messageText, MESSAGE_FLAGS, MessageColor::Type::System,
            FontStyle::ChatMediumSmall));
    this->message_ = std::move(singleLineMessage);
    this->createMessageLayout();
    this->layoutMessage();
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
        .selection = EMPTY_SELECTION,
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
    this->messageColors_.applyTheme(getTheme(), false, 255);
    this->messageColors_.regularBg = getTheme()->splits.input.background;
    if (this->messageLayout_)
    {
        this->messageLayout_->invalidateBuffer();
    }
}

void MessageView::scaleChangedEvent(float newScale)
{
    (void)newScale;

    this->layoutMessage();
}

void MessageView::layoutMessage()
{
    if (this->messageLayout_ == nullptr)
    {
        return;
    }

    bool updateRequired = this->messageLayout_->layout(
        {
            .messageColors = this->messageColors_,
            .flags = MESSAGE_FLAGS,
            .width = this->width_,
            .scale = this->scale(),
            .imageScale =
                this->scale() * static_cast<float>(this->devicePixelRatio()),
        },
        false);

    if (updateRequired)
    {
        this->setFixedSize(this->width_, this->messageLayout_->getHeight());
        this->update();
    }
}

}  // namespace chatterino
