#include "MessageView.hpp"

#include "Application.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "messages/Selection.hpp"
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

    this->signalHolder_.managedConnect(
        getIApp()->getWindows()->gifRepaintRequested, [&] {
            this->maybeUpdate();
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

    bool windowFocused = this->window() == QApplication::activeWindow();
    this->messageLayout_->paint(painter, this->width_, 0, 0, emptySelection,
                                false, windowFocused, false);
}

void MessageView::themeChangedEvent()
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

    auto flags = getIApp()->getWindows()->getWordFlags();
    bool updateRequired =
        this->messageLayout_->layout(this->width_, this->scale(), flags);

    if (updateRequired)
    {
        this->setFixedSize(this->width_, this->messageLayout_->getHeight());
        this->update();
    }
}

}  // namespace chatterino
