#include "notebookpagedroppreview.h"
#include "QPainter"
#include "colorscheme.h"

NotebookPageDropPreview::NotebookPageDropPreview(QWidget *parent = 0)
    : QWidget(parent)
{

}

void NotebookPageDropPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::getInstance().DropPreviewBackground);
}
