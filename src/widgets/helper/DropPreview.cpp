#include "widgets/helper/DropPreview.hpp"
#include "Application.hpp"
#include "singletons/Theme.hpp"

#include <QDebug>
#include <QPainter>

namespace chatterino {

NotebookPageDropPreview::NotebookPageDropPreview(BaseWidget *parent)
    : BaseWidget(parent)
//    , positionAnimation(this, "geometry")
{
    //    this->positionAnimation.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));
    this->setHidden(true);
}

void NotebookPageDropPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setPen(getApp()->themes->splits.dropPreviewBorder);
    painter.setBrush(getApp()->themes->splits.dropPreview);
    painter.drawRect(8, 8, this->width() - 17, this->height() - 17);
}

void NotebookPageDropPreview::hideEvent(QHideEvent *)
{
    //    this->animate = false;
}

void NotebookPageDropPreview::setBounds(const QRect &rect)
{
    //    if (rect == this->desiredGeometry) {
    //        return;
    //    }

    //    if (this->animate) {
    //        this->positionAnimation.stop();
    //        this->positionAnimation.setDuration(50);
    //        this->positionAnimation.setStartValue(this->geometry());
    //        this->positionAnimation.setEndValue(rect);
    //        this->positionAnimation.start();
    //    } else {
    this->setGeometry(rect);
    //    }

    //    this->desiredGeometry = rect;

    //    this->animate = true;
}

}  // namespace chatterino
