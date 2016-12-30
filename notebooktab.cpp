#include "notebook.h"
#include "notebooktab.h"
#include "QPainter"

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
{
    this->notebook = notebook;
    text = "<no title>";

    calcSize();
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

    QColor bg = QColor(255, 255, 255), fg = QColor(0, 0, 0);

//    if (selected)
//    {
//        bg = App.ColorScheme.TabSelectedBG;
//        text = App.ColorScheme.TabSelectedText;
//    }
//    else if (mouseOver)
//    {
//        bg = App.ColorScheme.TabHoverBG;
//        text = App.ColorScheme.TabHoverText;
//    }
//    else if ()
//    {
//        bg = App.ColorScheme.TabHighlightedBG;
//        text = App.ColorScheme.TabHighlightedText;
//    }
//    else if ()
//    {
//        bg = App.ColorScheme.TabNewMessageBG;
//        text = App.ColorScheme.TabHighlightedText;
//    }
//    else
//    {
//        bg = App.ColorScheme.TabBG;
//        text = App.ColorScheme.TabText;
//    }

    painter.fillRect(rect(), bg);

    painter.setPen(fg);
    painter.drawText(4, (height() + fontMetrics().height()) / 2, text);
}

void NotebookTab::mousePressEvent(QMouseEvent *)
{
    mouseDown = true;

    repaint();
}

void NotebookTab::mouseReleaseEvent(QMouseEvent *)
{
    mouseDown = false;

    repaint();

    notebook->select(page);
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
