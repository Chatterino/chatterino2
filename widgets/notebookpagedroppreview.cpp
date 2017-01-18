#include "widgets/notebookpagedroppreview.h"
#include "colorscheme.h"

#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookPageDropPreview::NotebookPageDropPreview(QWidget *parent)
    : QWidget(parent)
    , positionAnimation(this, "geometry")
    , desiredGeometry()
{
    setHidden(true);
}

void
NotebookPageDropPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(8, 8, width() - 17, height() - 17,
                     ColorScheme::instance().DropPreviewBackground);
}

void
NotebookPageDropPreview::setBounds(const QRect &rect)
{
    if (rect == this->desiredGeometry) {
        return;
    }

    this->positionAnimation.stop();
    this->positionAnimation.setDuration(50);
    this->positionAnimation.setStartValue(geometry());
    this->positionAnimation.setEndValue(rect);
    this->positionAnimation.start();

    this->desiredGeometry = rect;
}
}
}
