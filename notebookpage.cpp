#include "QWidget"
#include "QPainter"
#include "notebookpage.h"
#include "notebooktab.h"

NotebookPage::NotebookPage(NotebookTab *tab)
{
    this->tab = tab;
    tab->page = this;
}

void NotebookPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(0, 0, width() - 1, height() - 1);

    painter.drawText(8, 8, QString::number(tab->x()));
}
