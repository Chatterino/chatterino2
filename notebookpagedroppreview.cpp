#include "notebookpagedroppreview.h"
#include "QPainter"
#include "colorscheme.h"

NotebookPageDropPreview::NotebookPageDropPreview(QWidget *parent)
    : QWidget(parent)
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
