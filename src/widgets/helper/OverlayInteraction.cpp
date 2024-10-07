#include "widgets/helper/OverlayInteraction.hpp"

#include "common/Literals.hpp"
#include "widgets/OverlayWindow.hpp"

#include <QGridLayout>

namespace chatterino {

using namespace literals;

OverlayInteraction::OverlayInteraction(OverlayWindow *parent)
    : QWidget(nullptr)
    , interactAnimation_(this, "interactionProgress"_ba)
    , window_(parent)
{
    this->interactAnimation_.setStartValue(0.0);
    this->interactAnimation_.setEndValue(1.0);

    this->closeButton_.setButtonStyle(TitleBarButtonStyle::Close);
    this->closeButton_.setScaleIndependantSize(46, 30);
    this->closeButton_.hide();
    this->closeButton_.setCursor(Qt::PointingHandCursor);
}

void OverlayInteraction::attach(QGridLayout *layout)
{
    layout->addWidget(this, 0, 0);
    layout->addWidget(&this->closeButton_, 0, 0, Qt::AlignTop | Qt::AlignRight);
    layout->setContentsMargins(0, 0, 0, 0);

    QObject::connect(&this->closeButton_, &TitleBarButton::leftClicked,
                     [this]() {
                         this->window_->close();
                     });
}

QWidget *OverlayInteraction::closeButton()
{
    return &this->closeButton_;
}

void OverlayInteraction::startInteraction()
{
    if (this->interacting_)
    {
        return;
    }

    this->interacting_ = true;
    if (this->interactAnimation_.state() != QPropertyAnimation::Stopped)
    {
        this->interactAnimation_.stop();
    }
    this->interactAnimation_.setDirection(QPropertyAnimation::Forward);
    this->interactAnimation_.setDuration(100);
    this->interactAnimation_.start();
    this->window_->setOverrideCursor(Qt::SizeAllCursor);
    this->closeButton_.show();
}

void OverlayInteraction::endInteraction()
{
    if (!this->interacting_)
    {
        return;
    }

    this->interacting_ = false;
    if (this->interactAnimation_.state() != QPropertyAnimation::Stopped)
    {
        this->interactAnimation_.stop();
    }
    this->interactAnimation_.setDirection(QPropertyAnimation::Backward);
    this->interactAnimation_.setDuration(200);
    this->interactAnimation_.start();
    this->window_->setOverrideCursor(Qt::ArrowCursor);
    this->closeButton_.hide();
}

bool OverlayInteraction::isInteracting() const
{
    return this->interacting_;
}

void OverlayInteraction::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    QColor highlightColor(
        255, 255, 255, std::max(int(255.0 * this->interactionProgress()), 50));

    painter.setPen({highlightColor, 2});
    // outline
    auto bounds = this->rect();
    painter.drawRect(bounds);

    if (this->interactionProgress() <= 0.0)
    {
        return;
    }

    highlightColor.setAlpha(highlightColor.alpha() / 4);
    painter.setBrush(highlightColor);
    painter.setPen(Qt::transparent);

    // close button
    auto buttonSize = this->closeButton_.size();
    painter.drawRect(
        QRect{bounds.topRight() - QPoint{buttonSize.width(), 0}, buttonSize});
}

double OverlayInteraction::interactionProgress() const
{
    return this->interactionProgress_;
}

void OverlayInteraction::setInteractionProgress(double progress)
{
    this->interactionProgress_ = progress;
    this->update();
}

}  // namespace chatterino
