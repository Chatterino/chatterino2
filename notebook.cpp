#include "QWidget"
#include "notebook.h"
#include "notebookpage.h"

Notebook::Notebook(QWidget *parent)
    : QWidget(parent)
{
    auto list = new QList<std::tuple<NotebookPage*, NotebookTab*>>();

    this->pages = list;
}

NotebookPage*
Notebook::AddPage()
{
    auto page = new NotebookPage(this);
    auto tab = new NotebookTab(this, this, page);

    this->pages->append(std::make_tuple(page, tab));

    return page;
}
