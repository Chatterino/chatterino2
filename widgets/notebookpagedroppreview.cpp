#include "widgets/notebookpagedroppreview.h"
#include "colorscheme.h"

#include <QDebug>
#include <QPainter>

namespace  chatterino {
namespace  widgets {

NotebookPageDropPreview::NotebookPageDropPreview(QWidget *parent)
    : QWidget(parent)
    , positionAnimation(this, "geometry")
    , desiredGeometry()
    , animate(false)
{
    this->positionAnimation.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));
    this->setHidden(true);
}

void NotebookPageDropPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(8, 8, width() - 17, height() - 17,
                     ColorScheme::getInstance().DropPreviewBackground);
}

void NotebookPageDropPreview::hideEvent(QHideEvent *)
{
    animate = false;
}

void NotebookPageDropPreview::setBounds(const QRect &rect)
{
    if (rect == this->desiredGeometry) {
        return;
    }

    if (animate) {
        this->positionAnimation.stop();
        this->positionAnimation.setDuration(50);
        this->positionAnimation.setStartValue(this->geometry());
        this->positionAnimation.setEndValue(rect);
        this->positionAnimation.start();
    } else {
        this->setGeometry(rect);
    }

    this->desiredGeometry = rect;

    animate = true;
}
}
}
