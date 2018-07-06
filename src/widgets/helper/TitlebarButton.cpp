#include "TitlebarButton.hpp"

#include "singletons/Theme.hpp"

namespace chatterino {

TitleBarButton::TitleBarButton()
    : RippleEffectButton(nullptr)
{
}

TitleBarButton::Style TitleBarButton::getButtonStyle() const
{
    return this->style;
}

void TitleBarButton::setButtonStyle(Style _style)
{
    this->style = _style;
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

    switch (this->style) {
        case Minimize: {
            painter.fillRect(centerX - xD / 2, xD * 3 / 2, xD, 1, color);
            break;
        }
        case Maximize: {
            painter.setPen(color);
            painter.drawRect(centerX - xD / 2, xD, xD - 1, xD - 1);
            break;
        }
        case Unmaximize: {
            int xD2 = xD * 1 / 5;
            int xD3 = xD * 4 / 5;

            painter.drawRect(centerX - xD / 2 + xD2, xD, xD3, xD3);
            painter.fillRect(centerX - xD / 2, xD + xD2, xD3, xD3,
                             this->theme->window.background);
            painter.drawRect(centerX - xD / 2, xD + xD2, xD3, xD3);
            break;
        }
        case Close: {
            QRect rect(centerX - xD / 2, xD, xD - 1, xD - 1);
            painter.setPen(QPen(color, 1));

            painter.drawLine(rect.topLeft(), rect.bottomRight());
            painter.drawLine(rect.topRight(), rect.bottomLeft());
            break;
        }
        case User: {
            color = "#999";

            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::HighQualityAntialiasing);

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
        case Settings: {
            color = "#999";
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::HighQualityAntialiasing);

            painter.save();
            painter.translate(3, 3);

            auto a = xD / 3;
            QPainterPath path;

            path.arcMoveTo(a, a, 6 * a, 6 * a, 0 - (360 / 32.0));

            for (int i = 0; i < 8; i++) {
                path.arcTo(a, a, 6 * a, 6 * a, i * (360 / 8.0) - (360 / 32.0), (360 / 32.0));
                path.arcTo(2 * a, 2 * a, 4 * a, 4 * a, i * (360 / 8.0) + (360 / 32.0),
                           (360 / 32.0));
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

    RippleEffectButton::paintEvent(event);
    //    this->fancyPaint(painter);
}

}  // namespace chatterino
