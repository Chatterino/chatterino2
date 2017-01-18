#include "widgets/notebooktab.h"
#include "colorscheme.h"
#include "widgets/notebook.h"

#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
    , notebook(notebook)
    , text("<no title>")
    , selected(false)
    , mouseOver(false)
    , mouseDown(false)
    , highlightStyle(HighlightNone)
{
    calcSize();

    setAcceptDrops(true);
}

void
NotebookTab::calcSize()
{
    resize(fontMetrics().width(this->text) + 8, 24);
}

void
NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor fg = QColor(0, 0, 0);

    auto colorScheme = ColorScheme::instance();

    if (this->selected) {
        painter.fillRect(rect(), colorScheme.TabSelectedBackground);
        fg = colorScheme.TabSelectedText;
    } else if (this->mouseOver) {
        painter.fillRect(rect(), colorScheme.TabHoverBackground);
        fg = colorScheme.TabHoverText;
    } else if (this->highlightStyle == HighlightHighlighted) {
        painter.fillRect(rect(), colorScheme.TabHighlightedBackground);
        fg = colorScheme.TabHighlightedText;
    } else if (this->highlightStyle == HighlightNewMessage) {
        painter.fillRect(rect(), colorScheme.TabNewMessageBackground);
        fg = colorScheme.TabHighlightedText;
    } else {
        painter.fillRect(rect(), colorScheme.TabBackground);
        fg = colorScheme.TabText;
    }

    painter.setPen(fg);
    painter.drawText(4, (height() + fontMetrics().height()) / 2, this->text);
}

void
NotebookTab::mousePressEvent(QMouseEvent *)
{
    this->mouseDown = true;

    repaint();

    this->notebook->select(page);
}

void
NotebookTab::mouseReleaseEvent(QMouseEvent *)
{
    this->mouseDown = false;

    repaint();
}

void
NotebookTab::enterEvent(QEvent *)
{
    this->mouseOver = true;

    repaint();
}

void
NotebookTab::leaveEvent(QEvent *)
{
    this->mouseOver = false;

    repaint();
}

void
NotebookTab::dragEnterEvent(QDragEnterEvent *event)
{
    this->notebook->select(page);
}
}
}
