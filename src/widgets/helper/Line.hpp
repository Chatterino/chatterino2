#pragma once

#include "widgets/BaseWidget.hpp"

#include <QPainter>

namespace chatterino {

class Line : public BaseWidget
{
public:
    Line(bool vertical)
        : BaseWidget(nullptr)
        , vertical_(vertical)
    {
        if (this->vertical_)
        {
            this->setScaleIndependantWidth(8);
        }
        else
        {
            this->setScaleIndependantHeight(8);
        }
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);

        painter.setPen(QColor(153, 153, 153, 153));

        if (this->vertical_)
        {
            painter.drawLine(this->width() / 2, 0, this->width() / 2,
                             this->height());
        }
        else
        {
            painter.drawLine(0, this->height() / 2, this->width(),
                             this->height() / 2);
        }
    }

private:
    bool vertical_;
};

}  // namespace chatterino
