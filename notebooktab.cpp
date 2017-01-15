#include "notebooktab.h"
#include "colorscheme.h"
#include "notebook.h"

#include <QPainter>

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
    , m_notebook(notebook)
    , m_text("<no title>")
    , m_selected(false)
    , m_mouseOver(false)
    , m_mouseDown(false)
    , m_highlightStyle(HighlightNone)
{
    calcSize();

    setAcceptDrops(true);
}

void
NotebookTab::calcSize()
{
    resize(fontMetrics().width(m_text) + 8, 24);
}

void
NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor fg = QColor(0, 0, 0);

    auto colorScheme = ColorScheme::instance();

    if (m_selected) {
        painter.fillRect(rect(), colorScheme.TabSelectedBackground);
        fg = colorScheme.TabSelectedText;
    } else if (m_mouseOver) {
        painter.fillRect(rect(), colorScheme.TabHoverBackground);
        fg = colorScheme.TabHoverText;
    } else if (m_highlightStyle == HighlightHighlighted) {
        painter.fillRect(rect(), colorScheme.TabHighlightedBackground);
        fg = colorScheme.TabHighlightedText;
    } else if (m_highlightStyle == HighlightNewMessage) {
        painter.fillRect(rect(), colorScheme.TabNewMessageBackground);
        fg = colorScheme.TabHighlightedText;
    } else {
        painter.fillRect(rect(), colorScheme.TabBackground);
        fg = colorScheme.TabText;
    }

    painter.setPen(fg);
    painter.drawText(4, (height() + fontMetrics().height()) / 2, m_text);
}

void
NotebookTab::mousePressEvent(QMouseEvent *)
{
    m_mouseDown = true;

    repaint();

    m_notebook->select(page);
}

void
NotebookTab::mouseReleaseEvent(QMouseEvent *)
{
    m_mouseDown = false;

    repaint();
}

void
NotebookTab::enterEvent(QEvent *)
{
    m_mouseOver = true;

    repaint();
}

void
NotebookTab::leaveEvent(QEvent *)
{
    m_mouseOver = false;

    repaint();
}

void
NotebookTab::dragEnterEvent(QDragEnterEvent *event)
{
    m_notebook->select(page);
}
