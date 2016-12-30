#include "QWidget"
#include "notebookpage.h"
#include "notebooktab.h"

NotebookPage::NotebookPage(QWidget *parent, NotebookTab *tab)
    : QWidget(parent)
{
    this->tab = tab;
}
