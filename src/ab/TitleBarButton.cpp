#include "ab/TitleBarButton.hpp"

#include <QPainter>

namespace ab
{
    TitleBarButton::TitleBarButton()
    {
    }

    void TitleBarButton::setType(TitleBarButtonType type)
    {
        this->type_ = type;
        this->update();
    }

    TitleBarButtonType TitleBarButton::type() const
    {
        return this->type_;
    }

    // EVENTS
    void TitleBarButton::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);

        this->paint(painter);

        painter.setRenderHint(QPainter::Antialiasing, false);

        const auto color = this->palette().text().color();
        const auto xD = this->height() / 3;
        const auto centerX = this->width() / 2;

        switch (this->type_)
        {
            case TitleBarButtonType::Minimize:
            {
                painter.fillRect(centerX - xD / 2, xD * 3 / 2, xD, 1, color);
                break;
            }
            case TitleBarButtonType::Maximize:
            {
                painter.setPen(color);
                painter.drawRect(centerX - xD / 2, xD, xD - 1, xD - 1);
                break;
            }
            case TitleBarButtonType::Unmaximize:
            {
                int xD2 = xD * 1 / 5;
                int xD3 = xD * 4 / 5;

                painter.drawRect(centerX - xD / 2 + xD2, xD, xD3, xD3);
                painter.setCompositionMode(QPainter::CompositionMode_Source);
                painter.fillRect(
                    centerX - xD / 2, xD + xD2, xD3, xD3, QColor(0, 0, 0, 0));
                painter.drawRect(centerX - xD / 2, xD + xD2, xD3, xD3);
                break;
            }
            case TitleBarButtonType::Close:
            {
                QRect rect(centerX - xD / 2, xD, xD - 1, xD - 1);
                painter.setPen(QPen(color, 1));

                painter.drawLine(rect.topLeft(), rect.bottomRight());
                painter.drawLine(rect.topRight(), rect.bottomLeft());
                break;
            }
        }
    }
}  // namespace ab
