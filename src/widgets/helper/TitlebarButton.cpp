#include "widgets/helper/TitlebarButton.hpp"

#include "singletons/Theme.hpp"

#include <QPainterPath>

namespace chatterino {

TitleBarButton::TitleBarButton()
    : Button(nullptr)
{
}

TitleBarButtonStyle TitleBarButton::getButtonStyle() const
{
    return this->style_;
}

void TitleBarButton::setButtonStyle(TitleBarButtonStyle _style)
{
    this->style_ = _style;
    this->update();
}

void TitleBarButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.setOpacity(this->getCurrentDimAmount());

    QColor color = this->theme->window.text;
    QColor background = this->theme->window.background;

    int xD = this->height() / 3;
    int centerX = this->width() / 2;

    painter.setRenderHint(QPainter::Antialiasing, false);

    switch (this->style_)
    {
        case TitleBarButtonStyle::Minimize: {
            painter.fillRect(centerX - xD / 2, xD * 3 / 2, xD, 1, color);
            break;
        }
        case TitleBarButtonStyle::Maximize: {
            painter.setPen(color);
            painter.drawRect(centerX - xD / 2, xD, xD - 1, xD - 1);
            break;
        }
        case TitleBarButtonStyle::Unmaximize: {
            int xD2 = xD * 1 / 5;
            int xD3 = xD * 4 / 5;

            painter.drawRect(centerX - xD / 2 + xD2, xD, xD3, xD3);
            painter.fillRect(centerX - xD / 2, xD + xD2, xD3, xD3,
                             this->theme->window.background);
            painter.drawRect(centerX - xD / 2, xD + xD2, xD3, xD3);
            break;
        }
        case TitleBarButtonStyle::Close: {
            QRect rect(centerX - xD / 2, xD, xD - 1, xD - 1);
            painter.setPen(QPen(color, 1));

            painter.drawLine(rect.topLeft(), rect.bottomRight());
            painter.drawLine(rect.topRight(), rect.bottomLeft());
            break;
        }
        case TitleBarButtonStyle::User: {
            color = "#999";

            painter.setRenderHint(QPainter::Antialiasing);

            auto a = xD / 3;
            QPainterPath path;

            painter.save();
            painter.translate(3, 3);

            path.arcMoveTo(a, 4 * a, 6 * a, 6 * a, 0);
            path.arcTo(a, 4 * a, 6 * a, 6 * a, 0, 180);

            painter.fillPath(path, color);

            painter.setBrush(background);
            painter.drawEllipse(2 * a, 1 * a, 4 * a, 4 * a);

            painter.setBrush(color);
            painter.drawEllipse(2.5 * a, 1.5 * a, 3 * a + 1, 3 * a);
            painter.restore();

            break;
        }
        case TitleBarButtonStyle::Settings: {
            color = "#999";
            painter.setRenderHint(QPainter::Antialiasing);

            painter.save();
            painter.translate(3, 3);

            auto a = xD / 3;
            QPainterPath path;

            path.arcMoveTo(a, a, 6 * a, 6 * a, 0 - (360 / 32.0));

            for (int i = 0; i < 8; i++)
            {
                path.arcTo(a, a, 6 * a, 6 * a, i * (360 / 8.0) - (360 / 32.0),
                           (360 / 32.0));
                path.arcTo(2 * a, 2 * a, 4 * a, 4 * a,
                           i * (360 / 8.0) + (360 / 32.0), (360 / 32.0));
            }

            painter.strokePath(path, color);
            painter.fillPath(path, color);

            painter.setBrush(background);
            painter.drawEllipse(3 * a, 3 * a, 2 * a, 2 * a);
            painter.restore();
            break;
        }
        default:;
    }

    this->paintButton(painter);
}

void TitleBarButton::ncEnter()
{
    this->enterEvent(nullptr);
    this->update();
}

void TitleBarButton::ncLeave()
{
    this->leaveEvent(nullptr);
    this->update();
}

void TitleBarButton::ncMove(QPoint at)
{
    QMouseEvent evt(QMouseEvent::MouseMove, at, Qt::NoButton, Qt::NoButton,
                    Qt::NoModifier);
    this->mouseMoveEvent(&evt);
}

void TitleBarButton::ncMousePress(QPoint at)
{
    QMouseEvent evt(QMouseEvent::MouseButtonPress, at, Qt::LeftButton,
                    Qt::NoButton, Qt::NoModifier);
    this->mousePressEvent(&evt);
    this->update();
}

void TitleBarButton::ncMouseRelease(QPoint at)
{
    QMouseEvent evt(QMouseEvent::MouseButtonRelease, at, Qt::LeftButton,
                    Qt::NoButton, Qt::NoModifier);
    this->mouseReleaseEvent(&evt);
    this->update();
}

}  // namespace chatterino
