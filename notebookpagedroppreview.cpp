#include "notebookpagedroppreview.h"
#include "QPainter"
#include "colorscheme.h"

NotebookPageDropPreview::NotebookPageDropPreview(QWidget *parent)
    : QWidget(parent)
    , m_positionAnimation(this, "geometry")
    , m_desiredGeometry()
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
    if (rect == m_desiredGeometry) {
        return;
    }

    m_positionAnimation.stop();
    m_positionAnimation.setDuration(50);
    m_positionAnimation.setStartValue(geometry());
    m_positionAnimation.setEndValue(rect);
    m_positionAnimation.start();

    m_desiredGeometry = rect;
}
