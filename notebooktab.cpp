#include <QPainter>
#include "notebook.h"
#include "notebooktab.h"
#include "colorscheme.h"

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
{
    this->notebook = notebook;
    text = "<no title>";

    calcSize();

    setAcceptDrops(true);
}

int NotebookTab::getHighlightStyle()
{
    return highlightStyle;
}

void NotebookTab::setHighlightStyle(int style)
{
    highlightStyle = style;
    repaint();
}

void NotebookTab::setSelected(bool value)
{
    selected = value;
    repaint();
}

bool NotebookTab::getSelected()
{
    return selected;
}

void NotebookTab::calcSize()
{
    resize(fontMetrics().width(text) + 8, 24);
}

void NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor fg = QColor(0, 0, 0);

    auto colorScheme = ColorScheme::getInstance();

    if (selected)
    {
        painter.fillRect(rect(), colorScheme.TabSelectedBackground);
        fg = colorScheme.TabSelectedText;
    }
    else if (mouseOver)
    {
        painter.fillRect(rect(), colorScheme.TabHoverBackground);
        fg = colorScheme.TabHoverText;
    }
    else if (highlightStyle == HighlightHighlighted)
    {
        painter.fillRect(rect(), colorScheme.TabHighlightedBackground);
        fg = colorScheme.TabHighlightedText;
    }
    else if (highlightStyle == HighlightNewMessage)
    {
        painter.fillRect(rect(), colorScheme.TabNewMessageBackground);
        fg = colorScheme.TabHighlightedText;
    }
    else
    {
        painter.fillRect(rect(), colorScheme.TabBackground);
        fg = colorScheme.TabText;
    }

    painter.setPen(fg);
    painter.drawText(4, (height() + fontMetrics().height()) / 2, text);
}

void NotebookTab::mousePressEvent(QMouseEvent *)
{
    mouseDown = true;

    repaint();

    notebook->select(page);
}

void NotebookTab::mouseReleaseEvent(QMouseEvent *)
{
    mouseDown = false;

    repaint();
}

void NotebookTab::enterEvent(QEvent *)
{
    mouseOver = true;

    repaint();
}

void NotebookTab::leaveEvent(QEvent *)
{
    mouseOver = false;

    repaint();
}

void NotebookTab::dragEnterEvent(QDragEnterEvent *event)
{
    notebook->select(page);
}
