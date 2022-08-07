#include "DraggablePopup.hpp"

#include <QMouseEvent>

#include <chrono>

namespace chatterino {

namespace {

#ifdef Q_OS_LINUX
    FlagsEnum<BaseWindow::Flags> popupFlags{BaseWindow::Dialog,
                                            BaseWindow::EnableCustomFrame};
    FlagsEnum<BaseWindow::Flags> popupFlagsCloseAutomatically{
        BaseWindow::EnableCustomFrame};
#else
    FlagsEnum<BaseWindow::Flags> popupFlags{BaseWindow::EnableCustomFrame};
    FlagsEnum<BaseWindow::Flags> popupFlagsCloseAutomatically{
        BaseWindow::EnableCustomFrame, BaseWindow::Frameless,
        BaseWindow::FramelessDraggable};
#endif

}  // namespace

DraggablePopup::DraggablePopup(bool closeAutomatically, QWidget *parent)
    : BaseWindow(closeAutomatically ? popupFlagsCloseAutomatically : popupFlags,
                 parent)
    , lifetimeHack_(std::make_shared<bool>(false))
    , dragTimer_(this)

{
    if (closeAutomatically)
    {
        this->setActionOnFocusLoss(BaseWindow::Delete);
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
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        this->dragTimer_.start(std::chrono::milliseconds(17));
        this->startPosDrag_ = event->pos();
        this->movingRelativePos = event->localPos();
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
            (event->screenPos() - this->movingRelativePos).toPoint();
        this->isMoving_ = true;
    }
}

}  // namespace chatterino
