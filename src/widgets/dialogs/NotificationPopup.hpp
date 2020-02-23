#pragma once

#include "widgets/BaseWindow.hpp"

#include <QPixmap>

namespace chatterino {

class NotificationPopup : public BaseWindow
{
public:
    enum Location { TopLeft, TopRight, BottomLeft, BottomRight };
    NotificationPopup();

    void updatePosition();

    pajlada::Signals::Signal<QMouseEvent *> mouseRelease;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

}  // namespace chatterino
