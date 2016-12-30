#include "notebook.h"
#include "notebooktab.h"
#include "notebookpage.h"
#include "QPainter"

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
{
    this->notebook = notebook;

    resize(100, 24);
}

void NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setBrush(this->mouseDown ? QColor(0, 255, 0) : (this->mouseOver ? QColor(255, 0, 0) : QColor(0, 0, 255)));

    painter.drawRect(0, 0, this->width(), this->height());
}

void NotebookTab::mousePressEvent(QMouseEvent *)
{
    mouseDown = true;

    this->repaint();
}

void NotebookTab::mouseReleaseEvent(QMouseEvent *)
{
    mouseDown = false;

    this->repaint();
}

void NotebookTab::enterEvent(QEvent *)
{
    mouseOver = true;

    this->repaint();
}

void NotebookTab::leaveEvent(QEvent *)
{
    mouseOver = false;

    this->repaint();
}
