#include "NotificationPopup.hpp"

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "singletons/Settings.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QLabel>
#include <QScreen>

namespace chatterino {

NotificationPopup::NotificationPopup()
    : BaseWindow(BaseWindow::TopMost)
{
    this->setWindowFlags(
        Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint |
        Qt::BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
    this->setWindowOpacity(0.95);

    this->setScaleIndependantSize(360, 133);
}

void NotificationPopup::updatePosition()
{
    Location location = BottomRight;

    QDesktopWidget *desktop = QApplication::desktop();
    const QRect rect = desktop->availableGeometry();

    switch (location)
    {
        case BottomRight: {
            this->move(rect.right() - this->width(),
                       rect.bottom() - this->height());
        }
        break;
    }
}

void NotificationPopup::mouseReleaseEvent(QMouseEvent *event)
{
    mouseRelease.invoke(event);
    BaseWindow::mouseReleaseEvent(event);
}

}  // namespace chatterino
