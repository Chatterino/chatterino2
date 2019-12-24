#pragma once

#include <iostream>
#include <vector>
#include "widgets/BaseWindow.hpp"

#include <QPixmap>

namespace chatterino {

class NotificationPopup : public BaseWindow
{
public:
    enum Location { TopLeft, TopRight, BottomLeft, BottomRight };
    NotificationPopup();

    void setImageAndText(const QPixmap &image, const QString &text,
                         const QString &bottomText);
    void updatePosition();

    pajlada::Signals::Signal<QMouseEvent *> mouseDown;

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

}  // namespace chatterino
