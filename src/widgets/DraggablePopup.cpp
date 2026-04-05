// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/DraggablePopup.hpp"

#include "buttons/SvgButton.hpp"

#include <QMouseEvent>
#include <QWindow>

#include <chrono>

namespace chatterino {

namespace {

constexpr FlagsEnum<BaseWindow::Flags> POPUP_FLAGS{
#ifdef Q_OS_LINUX
    BaseWindow::Dialog,
#endif
    BaseWindow::EnableCustomFrame,
};
constexpr FlagsEnum<BaseWindow::Flags> POPUP_FLAGS_CLOSE_AUTOMATICALLY{
    BaseWindow::EnableCustomFrame,
    BaseWindow::Frameless,
    BaseWindow::FramelessDraggable,
};

}  // namespace

DraggablePopup::DraggablePopup(bool closeAutomatically, QWidget *parent)
    : BaseWindow(
          (closeAutomatically ? POPUP_FLAGS_CLOSE_AUTOMATICALLY : POPUP_FLAGS) |
              BaseWindow::DisableLayoutSave |
              BaseWindow::ClearBuffersOnDpiChange,
          parent)
    , lifetimeHack_(std::make_shared<bool>(false))
    , closeAutomatically_(closeAutomatically)
    , dragTimer_(this)

{
    if (closeAutomatically)
    {
        this->windowDeactivateAction = WindowDeactivateAction::Delete;
    }
    else
    {
        this->setAttribute(Qt::WA_DeleteOnClose);
    }

    // Update the window position according to this->requestedDragPos_ on every trigger
    this->dragTimer_.callOnTimeout(
        [this, hack = std::weak_ptr<bool>(this->lifetimeHack_)] {
            if (!hack.lock())
            {
                // Ensure this timer is never called after the object has been destroyed
                return;
            }

            if (!this->isMoving_)
            {
                return;
            }

            this->move(this->requestedDragPos_);
        });
}

void DraggablePopup::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton &&
        !this->windowHandle()->startSystemMove())
    {
        this->dragTimer_.start(std::chrono::milliseconds(17));
        this->startPosDrag_ = event->pos();
        this->movingRelativePos = event->position();
    }
}

void DraggablePopup::mouseReleaseEvent(QMouseEvent *event)
{
    this->dragTimer_.stop();
    this->isMoving_ = false;
}

void DraggablePopup::mouseMoveEvent(QMouseEvent *event)
{
    // Drag the window by the amount changed from inital position
    // Note that we provide a few *units* of deadzone so people don't
    // start dragging the window if they are slow at clicking.

    auto movePos = event->pos() - this->startPosDrag_;
    if (this->isMoving_ || movePos.manhattanLength() > 10.0)
    {
        this->requestedDragPos_ =
            (event->globalPosition() - this->movingRelativePos).toPoint();
        this->isMoving_ = true;
    }
}

void DraggablePopup::togglePinned()
{
    this->isPinned_ = !this->isPinned_;
    if (this->isPinned_)
    {
        this->windowDeactivateAction = WindowDeactivateAction::Nothing;
        this->pinButton_->setSource(this->pinEnabledSource_);
    }
    else
    {
        this->windowDeactivateAction = WindowDeactivateAction::Delete;
        this->pinButton_->setSource(this->pinDisabledSource_);
    }
}
Button *DraggablePopup::createPinButton()
{
    this->pinButton_ = new SvgButton(this->pinDisabledSource_, this, {3, 3});
    this->pinButton_->setScaleIndependentSize(18, 18);
    this->pinButton_->setToolTip("Pin Window");

    QObject::connect(this->pinButton_, &Button::leftClicked, this,
                     &DraggablePopup::togglePinned);
    return this->pinButton_;
}

bool DraggablePopup::ensurePinned()
{
    if (this->closeAutomatically_ && !this->isPinned_)
    {
        this->togglePinned();
        return true;
    }
    return false;
}

}  // namespace chatterino
