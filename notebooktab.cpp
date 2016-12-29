#include "notebook.h"
#include "notebooktab.h"
#include "notebookpage.h"

NotebookTab::NotebookTab(QWidget *parent, Notebook *notebook, NotebookPage *page)
    : QWidget(parent)
{
    this->notebook = notebook;
    this->page = page;
}
